/*----------LIBRERÍAS-----------------------------------------------------------------------------------------------------*/

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_image.h>

/*----------CONSTANTES----------------------------------------------------------------------------------------------------*/

#define CAS_X 9
#define CAS_Y 5
#define POS_X 1
#define LIM_SEM 7
#define LIM_F_PLANT 8
#define LIM_F_EXPLO 4
#define CANT_PLANT 8
#define RESOL_X 1280
#define RESOL_Y 720

/*----------CONSTANTES-PLANTAS--------------------------------------------------------------------------------------------*/

const short COST_PLANTA[CANT_PLANT + 1]	{ 0, 100, 50, 150,  50,  25, 175, 150, 200 };
const short PV_PLANTA[CANT_PLANT + 1]	{ 0,   8,  8,  20, 135,   6,   8,   8,   8 };
const short REC_PLANTA[CANT_PLANT + 1]	{ 0,  27, 33, 240, 120, 120,  40,  33,  40 };

/*----------VARIABLES_GLOBALES--------------------------------------------------------------------------------------------*/

static short planta_elegida{ 0 }, semillero_elegido{ 0 }, frames{};
static bool bucle{ 1 };
static short cont, soles_guard{ 5000 }, plantas_en_semillero[LIM_SEM];
static short cant_sol_tablero{};
static short cant_zombi[CAS_Y]{0,1,1,1,0}, cant_proy[CAS_Y];

/*----------ESTRUCTURAS---------------------------------------------------------------------------------------------------*/

struct Sol {
	short estado_act;
	short cant;
	short tiemp;
	float x, y;
	Sol* ant_sol, * sig_sol;
	union {
		//Diferentes estados del sol
		//ESTADO 0
		struct {
			float mov_x;
			float impulso;
			bool d;
		} anim;
		//ESTADO 3
		struct {
			short mov_x;
			short mov_y;
		} recol;
		//ESTADO -1
		struct {
			float mov_y;
		} cayendo;
	} estado;
}			static* sol_tablero;

struct Proyectil {
	short tiempo_ev;
	float x;
	Proyectil* ant_proy, * sig_proy;
	short tipo;
}	static* proyectil[CAS_Y];

struct Semillero {
	short plant, recarga, costo;
}	static semillero[LIM_SEM];

struct Zombie {
	short pos, pv, tiemp, estado, tiemp_estado, animacion;
}		static zombie[POS_X][CAS_Y];

struct Planta {
	short pos, pv, tiemp, estado, tiemp_evento, animacion;
}		static planta[CAS_X][CAS_Y];

/*----------PROTOTIPOS----------------------------------------------------------------------------------------------------*/

//INPUT
void registrar_teclas(ALLEGRO_EVENT);
void registrar_mouse(ALLEGRO_EVENT);

//PLANTAS
void eliminar_planta(Planta&);
void funcion_planta();
void generar_planta(short, Planta&);
void plantar_planta(ALLEGRO_EVENT);
void seleccionar_planta(ALLEGRO_EVENT, short);
short animacion_planta(short pos_x, short pos_y);

//SOL
bool funcion_sol(short, short, Sol&, short);
void generar_sol_recolect(short, short, short);
float animacion_sol(Sol&);

//PROYECTILES
void generar_proyectil(Proyectil*, int, short, short);
bool mover_proyectil(Proyectil&, short);

//EXTRA
void funcion_semillero();
void dibujar_numero(short, float, float, ALLEGRO_COLOR);

/*----------FUNCIÓN-MAIN--------------------------------------------------------------------------------------------------*/

int main() {
	srand(time(0));
	short mouse_x{}, mouse_y{};

	sol_tablero = new Sol;
	sol_tablero->ant_sol = sol_tablero;
	sol_tablero->sig_sol = NULL;
	sol_tablero->estado_act = 4;
	for (int i{}; i < CAS_Y; i++) {
		proyectil[i] = new Proyectil;
		proyectil[i]->ant_proy = proyectil[i];
		proyectil[i]->sig_proy = NULL;
	}

	//iniciar matriz plantas
	for (int y{}; y < CAS_Y; y++) {
		for (int x{}; x < CAS_X; x++) {
			planta[x][y].pos = 0;
			planta[x][y].pv = 0;
			planta[x][y].tiemp = 0;
			planta[x][y].estado = 0;
			planta[x][y].tiemp_evento = 0;
			planta[x][y].animacion = 0;
		}
	}

	for (int i{}, aleat; i < LIM_SEM; i++) {
		aleat = rand() % 8 + 1;
		for (int j{}; j <= i; j++) {
			if (aleat == semillero[j].plant) {
				aleat = rand() % 8 + 1;
				j = -1;
			}
		}
		semillero[i].plant = aleat;
		plantas_en_semillero[i] = aleat;
	}

	if (!al_init()) return -1;
	if (!al_init_image_addon()) return -1;
	if (!al_install_keyboard()) return -1;
	if (!al_install_mouse()) return -1;

	//al_set_new_display_flags(ALLEGRO_FULLSCREEN);

	ALLEGRO_DISPLAY* display = al_create_display(RESOL_X, RESOL_Y);
	ALLEGRO_EVENT_QUEUE* cola_eventos = al_create_event_queue();
	ALLEGRO_TIMER* tiempo = al_create_timer(1.0 / 60);
	ALLEGRO_EVENT eventos;
	ALLEGRO_BITMAP
		* plantas_dia_bitmap = al_load_bitmap("Sprites/Plants/Plants_Daytime.png"),
		* semillero_bitmap = al_load_bitmap("Sprites/Extra/Seedpackets_Bitmap.png"),
		* semillero_recarga = al_load_bitmap("Sprites/Extra/Delay_Seedpacket.png"),
		* pala_cursor = al_load_bitmap("Sprites/Plants/Shovel.png"),
		* pala_interfaz = al_load_bitmap("Sprites/Extra/Shovel_Hud.png"),
		* fondo_soles = al_load_bitmap("Sprites/Extra/Sun_Counter.png"),
		* sol_recol = al_load_bitmap("Sprites/Extra/Sun.png"),
		* explosion = al_load_bitmap("Sprites/Extra/Explosion_Red.png"),
		* guisante = al_load_bitmap("Sprites/Bullets/Bullet_Pea.png"),
		* zombie_basico = al_load_bitmap("Sprites/Zombies/Zombie_Basic.png"),
		* fondo_casa_dia = al_load_bitmap("Sprites/Daylight_Playground.png");

	al_set_window_title(display, "Plantas Contra Zombies Remake");

	al_register_event_source(cola_eventos, al_get_keyboard_event_source());
	al_register_event_source(cola_eventos, al_get_mouse_event_source());
	al_register_event_source(cola_eventos, al_get_timer_event_source(tiempo));

	std::cout << "INICIO DE PROGRAMA" << std::endl;
	al_start_timer(tiempo);

	while (bucle) {
		al_wait_for_event(cola_eventos, &eventos);
		//Registrar posición del mouse
		if (eventos.type == ALLEGRO_EVENT_MOUSE_AXES) {
			mouse_x = eventos.mouse.x;
			mouse_y = eventos.mouse.y;
		}
		switch (eventos.type) {
		//El juego avanza
		case ALLEGRO_EVENT_TIMER:
			if (!(frames % 15)) {
				Sol* ptr_sol = NULL, * anterior_sol = NULL;
				funcion_planta();
				funcion_semillero();
				ptr_sol = sol_tablero;
				for (int i{}; i < cant_sol_tablero; i++) {
					anterior_sol = ptr_sol;
					ptr_sol = ptr_sol->sig_sol;
					if (funcion_sol(mouse_x, mouse_y, *ptr_sol, i)) {
						ptr_sol = anterior_sol;
						i--;
					}
				}
			}
			if (!(frames % 2400)) {
				generar_sol_recolect(rand() % (RESOL_X - 180) + 180, 0, 25);
			}

			/*----------------------DIBUJADO------------------------------------------------------------------------------------------------------*/

			al_draw_bitmap(fondo_casa_dia, 0, 0, 0);
			al_draw_bitmap(fondo_soles, 10, -10, 0);
			al_draw_bitmap_region(pala_interfaz, 0, 0, 110, 110, LIM_SEM * 110 + 150, 0, 0);

			//Dibujar planta en el tablero

			for (int y{}; y < CAS_Y; y++) {
				for (int x{}; x < CAS_X; x++) {
					if (planta[x][y].pos) {
						if (planta[x][y].pos >= 0 && planta[x][y].pos <= 8) {
							al_draw_bitmap_region(plantas_dia_bitmap, (planta[x][y].pos - 1) * 90, animacion_planta(x, y), 90, 90, x * 100 + 195, y * 100 + 165, 0);
						}
						if (planta[x][y].pos == 3 && planta[x][y].estado == 1) {
							al_draw_bitmap_region(explosion, (int(planta[x][y].animacion / LIM_F_EXPLO)) * 300, 0, 300, 300, x * 100 + 95, y * 100 + 65, 0);
						}
					}
				}
			}

			//Dibujar proyectiles en el tablero
			for (int y{}; y < CAS_Y; y++) {
				Proyectil* ptr_proy = NULL, *ant_proy = NULL;
				ptr_proy = proyectil[y];
				for (int i{}; i < cant_proy[y]; i++) {
					ant_proy = ptr_proy;
					ptr_proy = ptr_proy->sig_proy;
					if (mover_proyectil(*ptr_proy, y)) {
						ptr_proy = ant_proy;
						i--;
						continue;
					}
					al_draw_bitmap_region(guisante, ptr_proy->tipo * 28, 0, 28, 28, ptr_proy->x, y * 100 + 189 + rand() % 3, 0);
				}
			}

			//Dibujar soles en el tablero
			if (cant_sol_tablero > 0) {
				Sol* ptr_sol = NULL;
				ptr_sol = sol_tablero;
				for (int i{}; i < cant_sol_tablero; i++) {
					ptr_sol = ptr_sol->sig_sol;
					if (ptr_sol->estado_act >= 0 && ptr_sol->estado_act <= 1) {
						al_draw_bitmap(sol_recol, ptr_sol->x + ptr_sol->estado.anim.mov_x, animacion_sol(*ptr_sol), 0);
					}
					else if (ptr_sol->estado_act == -1) {
						if (ptr_sol->y < ptr_sol->estado.cayendo.mov_y)
							ptr_sol->y += ptr_sol->estado.cayendo.mov_y / 500;
						al_draw_bitmap(sol_recol, ptr_sol->x, ptr_sol->y, 0);
					}
					else if (ptr_sol->estado_act != 3) {
						al_draw_bitmap(sol_recol, ptr_sol->x, ptr_sol->y, 0);
					}
					else {
						if (ptr_sol->x > 35 || ptr_sol->y > 5) {
							if (ptr_sol->x < ptr_sol->estado.recol.mov_x / 5 ||
								ptr_sol->y < ptr_sol->estado.recol.mov_y / 5) {
								ptr_sol->x -= ptr_sol->estado.recol.mov_x / 90;
								ptr_sol->y -= ptr_sol->estado.recol.mov_y / 90;
							}
							else if (ptr_sol->x < ptr_sol->estado.recol.mov_x / 3 ||
								ptr_sol->y < ptr_sol->estado.recol.mov_y / 3) {
								ptr_sol->x -= ptr_sol->estado.recol.mov_x / 50;
								ptr_sol->y -= ptr_sol->estado.recol.mov_y / 50;
							}
							else {
								ptr_sol->x -= ptr_sol->estado.recol.mov_x / 30;
								ptr_sol->y -= ptr_sol->estado.recol.mov_y / 30;
							}
						}
						al_draw_bitmap(sol_recol, ptr_sol->x, ptr_sol->y, 0);
					}
				}
			}

			al_draw_bitmap(fondo_soles, 10, -10, 0);
			dibujar_numero(soles_guard, 70, 80, al_map_rgba(255, 255, 255, 255));//Dibujar semillero

			for (int i{}; i < LIM_SEM; i++) {
				if (planta_elegida != semillero[i].plant) {
					al_draw_bitmap_region(semillero_bitmap, (semillero[i].plant - 1) * 110, 0, 110, 110, i * 110 + 150, 0, 0);
					if (semillero[i].recarga > 0) {
						al_draw_tinted_bitmap_region(semillero_recarga, al_map_rgba(90, 80, 90, 211), 0, 0, 110, (((float)semillero[i].recarga / REC_PLANTA[semillero[i].plant]) * 60 + 8), i * 110 + 150, 0, 0);
					}
					dibujar_numero(COST_PLANTA[semillero[i].plant], i * 110 + 215, 74, soles_guard >= COST_PLANTA[semillero[i].plant] ? al_map_rgb(255, 255, 255) : al_map_rgb(255, 31, 95));
				}
				else {
					al_draw_tinted_bitmap_region(semillero_bitmap, al_map_rgb(64, 64, 64), (semillero[i].plant - 1) * 110, 0, 110, 110, i * 110 + 150, 0, 0);
					dibujar_numero(COST_PLANTA[semillero[i].plant], i * 110 + 215, 74, al_map_rgb(64, 64, 64));
				}
			}

			al_draw_bitmap_region(pala_interfaz, 0, 0, 110, 110, LIM_SEM * 110 + 150, 0, 0);

			//Dibujar planta elegida en el cursor

			if (planta_elegida) {
				if (planta_elegida >= 1 && planta_elegida <= 8) {
					al_draw_tinted_bitmap_region(plantas_dia_bitmap, al_map_rgba(64, 64, 64, 165), (planta_elegida - 1) * 90, 0, 90, 90, mouse_x - 45, mouse_y - 45, 0);
				}
				else if (planta_elegida == -1) {
					al_draw_bitmap_region(pala_interfaz, 110, 0, 110, 110, LIM_SEM * 110 + 150, 0, 0);
					al_draw_bitmap(pala_cursor, mouse_x - 55, mouse_y - 55, 0);
				}
			}
			al_flip_display();
			frames++;
			break;

			//Registrar teclas
		case ALLEGRO_EVENT_KEY_DOWN:
			registrar_teclas(eventos);
			break;

			//Registrar botones mouse
		case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
			registrar_mouse(eventos);
			break;
		}
	}

	//LIBERACIÓN DE ESPACIO
	al_destroy_timer(tiempo);
	al_destroy_event_queue(cola_eventos);
	al_destroy_display(display);

	al_destroy_bitmap(fondo_casa_dia);
	al_destroy_bitmap(plantas_dia_bitmap);
	al_destroy_bitmap(semillero_bitmap);
	al_destroy_bitmap(pala_interfaz);
	al_destroy_bitmap(pala_cursor);
	al_destroy_bitmap(fondo_soles);
	al_destroy_bitmap(sol_recol);
	al_destroy_bitmap(semillero_recarga);
	al_destroy_bitmap(explosion);
	al_destroy_bitmap(guisante);
	al_destroy_bitmap(zombie_basico);

	std::cout << "ELIMINACION DE RESERVAS SOLES:" << std::endl;
	if (cant_sol_tablero) {
		Sol* elim_sol = NULL, * sig_elim_sol = NULL;
		elim_sol = sol_tablero;
		for (int i{}; i < cant_sol_tablero; i++) {
			sig_elim_sol = elim_sol->sig_sol;
			delete elim_sol;
			std::cout << "POSICION " << i << " ELIMINADA" << std::endl;
			elim_sol = sig_elim_sol;
		}
	}
	else {
		std::cout << "SOL TABLERO ELIMINADO" << std::endl;
		delete sol_tablero;
	}

	std::cout << "ELIMINACION DE GUISANTES EN LA PANTALLA DEL JUEGO" << std::endl;
	for (int i{}; i < CAS_Y; i++) {
		if (cant_proy[i]) {
			Proyectil* elim_proy = NULL, * sig_elim_proy = NULL;
			elim_proy = proyectil[i];
			for (int j{}; j < cant_proy[i]; j++) {
				sig_elim_proy = elim_proy->sig_proy;
				delete elim_proy;
				std::cout << "POSICION " << j << " DE " << i << " ELIMINADA" << std::endl;
				elim_proy = sig_elim_proy;
			}
		}else{
			std::cout << "PROYECTIL" << i << "ELIMINADO" << std::endl;
			delete proyectil[i];
		}
	}
}

/*----------FUNCIONES-INPUT-----------------------------------------------------------------------------------------------*/

void registrar_mouse(ALLEGRO_EVENT mouse) {
	switch (mouse.mouse.button) {
	case ALLEGRO_MOUSE_BUTTON_LEFT:
		if (mouse.mouse.y >= 160) {
			plantar_planta(mouse);
		}
		else {
			seleccionar_planta(mouse, 0);
		}
		break;
	}
}

void registrar_teclas(ALLEGRO_EVENT teclado) {
	switch (teclado.keyboard.keycode) {
	case ALLEGRO_KEY_P:
		for (int y{}; y < CAS_Y; y++) {
			for (int x{}; x < CAS_X; x++) {
				planta[x][y].pos = 0;
			}
		}
		break;
	case ALLEGRO_KEY_ALT:
		for (int i{}, aleat; i < LIM_SEM; i++) {
			aleat = rand() % 8 + 1;
			for (int j{}; j <= i; j++) {
				if (aleat == semillero[j].plant) {
					aleat = rand() % 8 + 1;
					j = -1;
				}
			}
			semillero[i].plant = aleat;
			plantas_en_semillero[i] = aleat;
		}
		break;
	case ALLEGRO_KEY_1: seleccionar_planta(teclado, 1); break;
	case ALLEGRO_KEY_2: seleccionar_planta(teclado, 2); break;
	case ALLEGRO_KEY_3: seleccionar_planta(teclado, 3); break;
	case ALLEGRO_KEY_4: seleccionar_planta(teclado, 4); break;
	case ALLEGRO_KEY_5: seleccionar_planta(teclado, 5); break;
	case ALLEGRO_KEY_6: seleccionar_planta(teclado, 6); break;
	case ALLEGRO_KEY_7: seleccionar_planta(teclado, 7); break;
	case ALLEGRO_KEY_L: seleccionar_planta(teclado, -1); break;
	case ALLEGRO_KEY_ESCAPE: bucle = 0; break;
	}
}

/*----------FUNCIONES-PLANTAS---------------------------------------------------------------------------------------------*/

void eliminar_planta(Planta& planta) {
	planta.pos = 0;
	planta.pv = 0;
	planta.tiemp = 0;
	planta.estado = 0;
	planta.tiemp_evento = 0;
	planta.animacion = 0;
}

void funcion_planta() {
	for (int x{}; x < CAS_X; x++) {
		for (int y{}; y < CAS_Y; y++) {
			if (planta[x][y].pos) {
				if (planta[x][y].pv <= 0) {
					eliminar_planta(planta[x][y]);
					continue;
				}
				switch (planta[x][y].pos) {
				case 1://LANZAGUISANTES
					if (planta[x][y].estado == 0 && cant_zombi[y] > 0 && planta[x][y].tiemp >= 5 + rand() % 2) {
						planta[x][y].tiemp = -1;
						planta[x][y].estado = 1;
						planta[x][y].animacion = 0;
					}
					if (planta[x][y].estado == 1) {
						if (planta[x][y].tiemp >= 1) {
							generar_proyectil(proyectil[y], y, planta[x][y].pos, x);
							planta[x][y].tiemp = -1;
							planta[x][y].animacion = 2 * LIM_F_PLANT;
							planta[x][y].estado = 0;
						}
					}
					break;
				case 2://GIRASOL
					if (planta[x][y].estado == 1) {
						//Generar soles
						if (planta[x][y].tiemp >= 6) {
							planta[x][y].animacion = rand() % 100;
							planta[x][y].tiemp = 0;
							planta[x][y].estado = 0;
							generar_sol_recolect(x * 100 + 195, y * 100 + 165, 25);
						}
					}
					else if (planta[x][y].estado == 0) {
						//cambio de estado: soles
						if (planta[x][y].tiemp >= 54 + rand() % 8) {
							planta[x][y].animacion = -1;
							planta[x][y].tiemp = 0;
							planta[x][y].estado = 1;
							std::cout << "SOL GENERANDO" << std::endl;
						}
					}
					break;
				case 3://PETACEREZA
					if (planta[x][y].estado == 0 && planta[x][y].animacion >= 3 * LIM_F_PLANT * 2 - 1) {
						planta[x][y].animacion = 0;
						planta[x][y].estado = 1;
					}
					else if (planta[x][y].estado == 1 && planta[x][y].tiemp >= LIM_F_EXPLO * 1.5) {
						planta[x][y].pv = 0;
					}
					break;
				case 5://PAPAPUM
					if (planta[x][y].estado == 0 && planta[x][y].tiemp >= 80) {
						planta[x][y].animacion = 0;
						planta[x][y].tiemp = 0;
						planta[x][y].estado = 1;
						std::cout << "PAPAPUM ARMADA" << std::endl;
					}
					break;
				case 6://Hielaguisantes
					if (planta[x][y].estado == 0 && cant_zombi[y] > 0 && planta[x][y].tiemp >= 5 + rand() % 2) {
						planta[x][y].tiemp = -1;
						planta[x][y].estado = 1;
						planta[x][y].animacion = 0;
					}
					if (planta[x][y].estado == 1) {
						if (planta[x][y].tiemp >= 1) {
							generar_proyectil(proyectil[y], y, planta[x][y].pos, x);
							planta[x][y].tiemp = -1;
							planta[x][y].animacion = 2 * LIM_F_PLANT;
							planta[x][y].estado = 0;
						}
					}
					break;
				case 8://Repetidora
					if (planta[x][y].estado == 0 && cant_zombi[y] > 0 && planta[x][y].tiemp >= 5 + rand() % 2) {
						planta[x][y].tiemp = -1;
						planta[x][y].estado = 1;
						planta[x][y].tiemp_evento = 0;
						planta[x][y].animacion = 0;
					}
					if (planta[x][y].estado == 1) {
						if (planta[x][y].tiemp >= 1) {
							generar_proyectil(proyectil[y], y, planta[x][y].pos, x);
							planta[x][y].tiemp = 0;
							planta[x][y].animacion = 2 * LIM_F_PLANT - LIM_F_PLANT / 2;
							planta[x][y].tiemp_evento++;
						}
						if (planta[x][y].tiemp_evento >= 2) {
							planta[x][y].animacion = 2 * LIM_F_PLANT;
							planta[x][y].tiemp = -1;
							planta[x][y].estado = 0;
						}
					}
					break;
				default:
					break;
				}
				planta[x][y].tiemp++;
			}
		}
	}
}

void generar_planta(short id_planta, Planta& planta) {
	planta.pv = PV_PLANTA[id_planta];
	planta.pos = id_planta;
	planta.tiemp = 0;
	planta.estado = 0;
	planta.tiemp_evento = 0;
	planta.animacion = 0;
	std::cout << "PV[" << id_planta << "] = " << planta.pv << std::endl;
}

void plantar_planta(ALLEGRO_EVENT pos) {
	short pos_x{ -1 }, pos_y{ -1 };
	for (int i{}; i < CAS_X; i++) {
		if (pos.mouse.x >= 190 + i * 100 && pos.mouse.x < 290 + i * 100) {
			pos_x = i;
		}
	}
	for (int i{}; i < CAS_Y; i++) {
		if (pos.mouse.y >= 160 + i * 100 && pos.mouse.y < 260 + i * 100) {
			pos_y = i;
		}
	}
	if (pos_x != -1 && pos_y != -1) {
		//PLANTAR UNA PLANTA
		if (planta_elegida > 0) {
			if (planta_elegida > 0 && !planta[pos_x][pos_y].pos && soles_guard >= COST_PLANTA[planta_elegida]) {
				soles_guard -= COST_PLANTA[planta_elegida];
				semillero[semillero_elegido].recarga = REC_PLANTA[semillero[semillero_elegido].plant];
				generar_planta(planta_elegida, planta[pos_x][pos_y]);
				std::cout << "PLANTA " << planta_elegida << " PLANTADA EN \t[" << pos_x << "][" << pos_y << "]" << std::endl;

			}
		}
		//DESPLANTAR UNA PLANTA
		else if (planta_elegida == -1 && planta[pos_x][pos_y].pos > 0) {
			eliminar_planta(planta[pos_x][pos_y]);
			std::cout << "ASESINATO EN \t\t[" << pos_x << "][" << pos_y << "]" << std::endl;
		}
		planta_elegida = 0;
	}
}

void seleccionar_planta(ALLEGRO_EVENT pos, short pos_semillero) {
	short pos_x{ -1 };
	for (int i{}; i <= LIM_SEM; i++) {
		if (pos.mouse.x >= 150 + i * 110 && pos.mouse.x < 260 + i * 110) {
			pos_x = i;
		}
	}
	if (pos_semillero) {
		if (pos_semillero > 0)
			pos_x = pos_semillero - 1;
		else
			pos_x = LIM_SEM;
	}
	if (pos_x != -1 && pos.mouse.y <= 110) {
		if (!planta_elegida) {
		ASIGNAR_PLANTA:
			if (pos_x != LIM_SEM) {
				if (soles_guard >= COST_PLANTA[semillero[pos_x].plant] && semillero[pos_x].recarga <= 0) {
					planta_elegida = semillero[pos_x].plant;
					semillero_elegido = pos_x;
				}
			}
			else planta_elegida = -1;
		}
		else if (planta_elegida == semillero[pos_x].plant || (planta_elegida == -1 && pos_x == LIM_SEM)) {
			planta_elegida = 0;
		}
		else {
			goto ASIGNAR_PLANTA;
		}
	}
}

short animacion_planta(short x, short y) {
	switch (planta[x][y].pos) {
	case 1://LANZAGUISANTES
		switch (planta[x][y].estado) {
		case 0://NORMAL
			if (planta[x][y].animacion < 8 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			else if (planta[x][y].animacion >= 8 * LIM_F_PLANT - 1) planta[x][y].animacion = 0;
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0: case 4:
				return 0;
			case 1: case 3:
				return 90;
			case 2:
				return 180;
			case 5: case 7:
				return 270;
			case 6:
				return 360;
			}
			break;
		case 1:
			if (planta[x][y].animacion < 4 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0: return 0;
			case 1: return 450;
			case 2: return 540;
			default: return 630;
			}
			break;
		}
		break;
	case 2://GIRASOL
		//animación planta
		switch (planta[x][y].estado) {
		case 0://NORMAL
			if (planta[x][y].animacion < 8 * (LIM_F_PLANT * 1.5) - 1) planta[x][y].animacion++;
			else if (planta[x][y].animacion >= 8 * (LIM_F_PLANT * 1.5) - 1) planta[x][y].animacion = 0;
			switch (int(planta[x][y].animacion / (LIM_F_PLANT * 1.5))) {
			case 0: case 4:
				return 0;//FRAME 1
			case 1: case 3:
				return 90;//FRAME 2
			case 2:
				return 180;//FRAME 3
			case 5: case 7:
				return 270;//FRAME 4
			case 6:
				return 360;//FRAME 5
			}
		case 1://ANIMACIÓN SOLES
			if (planta[x][y].animacion < 2 && !(frames % 15)) planta[x][y].animacion++;
			else if (planta[x][y].animacion == 2 && planta[x][y].tiemp >= 6) planta[x][y].animacion++;
			return planta[x][y].animacion * 90 + 450;
		}
		break;
	case 3://PETACEREZA
		switch (planta[x][y].estado) {
		case 0:
			if (planta[x][y].animacion < 3 * (LIM_F_PLANT * 2) - 1) planta[x][y].animacion++;
			return (int(planta[x][y].animacion / (LIM_F_PLANT * 2)) * 90);
			break;
		case 1:
			if (planta[x][y].animacion < 12 * LIM_F_EXPLO) planta[x][y].animacion++;
			return 360;
		}
		break;
	case 4://NUEZ
		if (planta[x][y].pv > 53) {
			return 0 + (planta[x][y].tiemp % 8 ? 0 : 1) * 360;
		}
		else if (planta[x][y].pv > 31 && planta[x][y].pv <= 53) {
			return 90 + (planta[x][y].tiemp % 7 ? 0 : 1) * 360;
		}
		else if (planta[x][y].pv > 10 && planta[x][y].pv <= 31) {
			return 180 + (planta[x][y].tiemp % 6 ? 0 : 1) * 360;
		}
		else {
			return 270 + (planta[x][y].tiemp % 3 ? 0 : 1) * 360;
		}
		break;
	case 5://PAPAPUM
		switch (planta[x][y].estado) {
		case 0:
			//NORMAL
			return ((planta[x][y].tiemp >= 65 ? planta[x][y].tiemp : 0) % 2) * 90 + 180;
		case 1:
			//ACTIVA
			planta[x][y].animacion++;
			switch (int(planta[x][y].animacion / 8)) {
			case 0:
				return 360 + (planta[x][y].tiemp % 2 * 270);
			case 1:
				return 450 + (planta[x][y].tiemp % 2 * 270);
			case 3: case 4:
				return 540 + (planta[x][y].tiemp % 2 * 270);
			default:
				return (planta[x][y].tiemp % 2) * 90;
			}
		}
		break;
	case 6://HIELAGUISANTES
		switch (planta[x][y].estado) {
		case 0://NORMAL
			if (planta[x][y].animacion < 8 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			else if (planta[x][y].animacion >= 8 * LIM_F_PLANT - 1) planta[x][y].animacion = 0;
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0: case 4:
				return 0;
			case 1: case 3:
				return 90;
			case 2:
				return 180;
			case 5: case 7:
				return 270;
			case 6:
				return 360;
			}
			break;
		case 1:
			if (planta[x][y].animacion < 4 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0: return 0;
			case 1: return 450;
			case 2: return 540;
			default: return 630;
			}
			break;
		}
		break;
	case 7://CARROÑIVORA
		switch (planta[x][y].estado) {
		case 0://NORMAL
			if (planta[x][y].animacion < 8 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			else if (planta[x][y].animacion >= 8 * LIM_F_PLANT - 1) planta[x][y].animacion = 0;
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0: case 4:
				return 0;
			case 1: case 3:
				return 90;
			case 2:
				return 180;
			case 5: case 7:
				return 270;
			case 6:
				return 360;
			}
			break;
		}
		break;
	case 8://REPETIDORA
		switch (planta[x][y].estado) {
		case 0://NORMAL
			if (planta[x][y].animacion < 8 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			else if (planta[x][y].animacion >= 8 * LIM_F_PLANT - 1) planta[x][y].animacion = 0;
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0: case 4:
				return 0;
			case 1: case 3:
				return 90;
			case 2:
				return 180;
			case 5: case 7:
				return 270;
			case 6:
				return 360;
			}
			break;
		case 1:
			if (planta[x][y].animacion < 4 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0: return 0;
			case 1: return 450;
			case 2: return 540;
			default: return 630;
			}
			break;
		}
		break;
	}
	return 0;
}

/*----------FUNCIONES-SOL-------------------------------------------------------------------------------------------------*/

bool funcion_sol(short mouse_x, short mouse_y, Sol& sol, short pos) {
	short x, y;
	if (sol.estado_act < 3) {
		switch (sol.estado_act) {
		case 0:
			y = (short)animacion_sol(sol);
			x = sol.x + sol.estado.anim.mov_x;
			break;
		case 1:
			y = (short)animacion_sol(sol);
			x = sol.x + (short)sol.estado.anim.mov_x;
			sol.x = x;
			sol.y = y;
			sol.estado_act = 2;
			break;
		case -1:
			x = sol.x;
			y = sol.y;
			if (y > sol.estado.cayendo.mov_y)
				sol.estado_act = 2;
		default:
			x = sol.x;
			y = sol.y;
			sol.tiemp++;
			break;
		}
		if (mouse_x > x - 160 && mouse_x < x + 160 && mouse_y > y - 160 && mouse_y < y + 160) {
			std::cout << "SOL RECOLECTADO" << std::endl;
			soles_guard += sol.cant;
			sol.x = x;
			sol.y = y;
			sol.estado.recol.mov_x = x - 35;
			sol.estado.recol.mov_y = y - 5;
			sol.estado_act = 3;
		}
		if (sol.estado_act == 2 && sol.tiemp >= 150) {
			goto Eliminar_sol;
		}
	}
	else if (sol.estado_act == 3) {
		if (sol.x <= 35 || sol.y <= 5) {
		Eliminar_sol:
			if (sol.sig_sol) {
				Sol* anterior_sol = NULL, * siguiente_sol = NULL;

				siguiente_sol = sol.sig_sol;
				std::cout << "ANTERIOR APUNTADO: " << sol.sig_sol << std::endl;
				std::cout << "NUEVO ANT APUNTADO: " << sol.ant_sol->sig_sol << std::endl;
				sol.ant_sol->sig_sol = siguiente_sol;

				anterior_sol = sol.ant_sol;
				std::cout << "SIGUIENTE APUNTADO: " << sol.ant_sol << std::endl;
				std::cout << "NUEVO SIG APUNTADO: " << sol.sig_sol->ant_sol << std::endl;
				sol.sig_sol->ant_sol = anterior_sol;

				if (anterior_sol == siguiente_sol) {
					std::cout << "ERROR, ANTERIOR Y SIGUIENTE SOL SON IGUALES" << std::endl;
				}

			}
			else {
				sol.ant_sol->sig_sol = NULL;
			}
			delete& sol;
			cant_sol_tablero--;
			return 1;
		}
	}
	return 0;
}

float animacion_sol(Sol& sol) {
	short direccion = sol.estado.anim.d * 2 - 1;
	if (direccion == 1) {
		if (sol.estado.anim.mov_x < sol.estado.anim.impulso * 2) {
			sol.estado.anim.mov_x += sol.estado.anim.impulso / (sol.estado.anim.impulso / 6 * 3);
		}
		if (sol.estado.anim.mov_x >= sol.estado.anim.impulso * 2) {
			sol.estado.anim.mov_x = sol.estado.anim.impulso * 2;
			sol.estado_act = 1;
		}
	}
	else if (direccion == -1) {
		if (sol.estado.anim.mov_x > sol.estado.anim.impulso * -2) {
			sol.estado.anim.mov_x -= sol.estado.anim.impulso / (sol.estado.anim.impulso / 6 * 3);
		}
		if (sol.estado.anim.mov_x <= sol.estado.anim.impulso * -2) {
			sol.estado.anim.mov_x = sol.estado.anim.impulso * -2;
			sol.estado_act = 1;
		}
	}
	return sol.y - sqrt(pow(sol.estado.anim.impulso, 2) - pow(sol.estado.anim.mov_x + sol.estado.anim.impulso * (-direccion), 2));
}

void generar_sol_recolect(short pos_x, short pos_y, short cant) {
	Sol* nuevo_sol = NULL, * anterior_sol = NULL, * ptr_sol = NULL;
	nuevo_sol = anterior_sol = sol_tablero;
	for (int i{}; i < cant_sol_tablero; i++) {
		ptr_sol = nuevo_sol->sig_sol;
		nuevo_sol = ptr_sol;
	}
	//Generar nuevo espacio en memoria
	nuevo_sol->sig_sol = new Sol;
	//asignar nuevo espacio en memoria
	anterior_sol = nuevo_sol;
	nuevo_sol = nuevo_sol->sig_sol;

	//Asignar punteros
	nuevo_sol->ant_sol = anterior_sol;
	nuevo_sol->sig_sol = NULL;

	nuevo_sol->cant = cant;
	nuevo_sol->tiemp = 0;
	if (pos_y) {
		nuevo_sol->estado_act = 0;
		nuevo_sol->x = pos_x + 22;
		nuevo_sol->y = pos_y + 60 - rand() % 6 * 12;
		nuevo_sol->estado.anim.impulso = (rand() % 5 + 1) * 5 + 20;
		nuevo_sol->estado.anim.d = rand() % 2;
		nuevo_sol->estado.anim.mov_x = 0;
	}
	else {
		nuevo_sol->estado_act = -1;
		nuevo_sol->x = pos_x;
		nuevo_sol->y = 0;
		nuevo_sol->estado.cayendo.mov_y = rand() % (RESOL_Y - 500) + 200;
	}
	cant_sol_tablero++;
}

/*----------FUNCIONES-PROYECTIL-------------------------------------------------------------------------------------------*/

void generar_proyectil(Proyectil *proyectil, int y, short tipo, short pos_x) {
	Proyectil* nuevo_proy = NULL, * anterior_proy = NULL, * ptr_proy = NULL;
	nuevo_proy = anterior_proy = proyectil;
	for (int i{}; i < cant_proy[y]; i++) {
		ptr_proy = nuevo_proy->sig_proy;
		nuevo_proy = ptr_proy;
	}
	//Generar nuevo espacio en memoria
	nuevo_proy->sig_proy = new Proyectil;
	//asignar nuevo espacio en memoria
	anterior_proy = nuevo_proy;
	nuevo_proy= nuevo_proy->sig_proy;

	//Asignar punteros
	nuevo_proy->ant_proy = anterior_proy;
	nuevo_proy->sig_proy = NULL;

	nuevo_proy->x = pos_x * 100 + 280;
	switch (tipo) {
	default:
		nuevo_proy->tipo = 0;
		break;
	case 6:
		nuevo_proy->tipo = 1;
		break;
	}
	nuevo_proy->tiempo_ev = 0;
	cant_proy[y]++;
}


bool mover_proyectil(Proyectil& proy, short pos_y) {
	if (proy.x > RESOL_X) {
		Eliminar_Proyectil:
		if (proy.sig_proy) {
			proy.ant_proy->sig_proy = proy.sig_proy;
			proy.sig_proy->ant_proy = proy.ant_proy;
		}
		else {
			proy.ant_proy->sig_proy = NULL;
		}
		cant_proy[pos_y]--;
		std::cout << "POSICION ELIMINADA: " << &proy << std::endl;
		delete& proy;
		return 1;
	}
	proy.x += 5;
	return 0;
}


/*----------FUNCIONES-EXTRA-----------------------------------------------------------------------------------------------*/

void dibujar_numero(short num, float x, float y, ALLEGRO_COLOR color) {
	short tam{}, copi_num{ num };
	float pos_x;
	ALLEGRO_BITMAP* numeros = al_load_bitmap("Sprites/Extra/Number_Font.png");
	if (copi_num == 0) {
		tam++;
	}
	else {
		while (copi_num > 0) {
			copi_num /= 10;
			tam++;
		}
	}
	copi_num = num;
	pos_x = 10 * tam - 20;
	for (int i{ 1 }; i <= tam; i++, copi_num /= 10, pos_x -= 20) {
		switch (copi_num % 10) {
		case 0: al_draw_tinted_bitmap_region(numeros, color, 90, 0, 30, 30, x + pos_x, y, 0); break;
		case 1: al_draw_tinted_bitmap_region(numeros, color, 00, 0, 30, 30, x + pos_x, y, 0); break;
		case 2: al_draw_tinted_bitmap_region(numeros, color, 30, 0, 30, 30, x + pos_x, y, 0); break;
		case 3: al_draw_tinted_bitmap_region(numeros, color, 60, 0, 30, 30, x + pos_x, y, 0); break;
		case 4: al_draw_tinted_bitmap_region(numeros, color, 00, 30, 30, 30, x + pos_x, y, 0); break;
		case 5: al_draw_tinted_bitmap_region(numeros, color, 30, 30, 30, 30, x + pos_x, y, 0); break;
		case 6: al_draw_tinted_bitmap_region(numeros, color, 60, 30, 30, 30, x + pos_x, y, 0); break;
		case 7: al_draw_tinted_bitmap_region(numeros, color, 00, 60, 30, 30, x + pos_x, y, 0); break;
		case 8: al_draw_tinted_bitmap_region(numeros, color, 30, 60, 30, 30, x + pos_x, y, 0); break;
		case 9: al_draw_tinted_bitmap_region(numeros, color, 60, 60, 30, 30, x + pos_x, y, 0); break;
		}
	}
	al_destroy_bitmap(numeros);
}

void funcion_semillero() {
	for (int i{}; i < LIM_SEM; i++) {
		semillero[i].recarga--;
	}
}