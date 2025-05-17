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
#define LIM_F_ZOMB 28
#define LIM_F_EXPLO 4

#define CANT_PLANT 8
#define RESOL_X 1280
#define RESOL_Y 720

#define NORM_C al_map_rgb(255, 255, 255)
#define TRANS_C al_map_rgba(204, 204, 204, 204)

/*----------CONSTANTES-PLANTAS--------------------------------------------------------------------------------------------*/

const short COST_PLANTA[CANT_PLANT + 1]	{ 0, 100, 50, 150,  50,  25, 175, 150, 200 };
const short PV_PLANTA[CANT_PLANT + 1]	{ 0,  20, 20,  20, 350,  15,  20,  20,  20 };
const short REC_PLANTA[CANT_PLANT + 1]	{ 0,  60, 90, 600, 300, 300, 100,  90, 100 };

/*----------VARIABLES_GLOBALES--------------------------------------------------------------------------------------------*/

static short planta_elegida{ 0 }, semillero_elegido{ 0 }, frames{};
static bool bucle{ 1 };
static short cont, soles_guard{ 5000 }, soles_guard_suma{ 0 }, plantas_en_semillero[LIM_SEM];
static short cant_sol_tablero{}, cant_particulas{};
static short cant_zombi[CAS_Y]{0,0,0,0,0}, cant_proy[CAS_Y], anim_sobre_tablero{ 2 };

/*----------ESTRUCTURAS---------------------------------------------------------------------------------------------------*/

struct Sol {
	short estado_act;
	short cant;
	short angulo;
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
}	static  semillero[LIM_SEM];

struct Zombie {
	short id;
	short pv;
	short tiemp;
	short estado; 
	short est_danio;
	short animacion, prior;
	short anim_danio;
	float x;
	bool comiendo;
	Zombie* ant_zomb, * sig_zomb;
}		static* zombie[CAS_Y];

struct Planta {
	short pos, pv, estado, tiemp_evento;
	int tiemp, animacion;
	short anim_danio;
}		static  planta[CAS_X][CAS_Y];

struct Oleada {
	short dificultad = 1;
	short limite_dificultad = 50;
	short ritmo_niv = 100;
	short zomb_atak;
	short tiempNoZomb;
	short tiempo;
	short oleada;
	short tiempo_Oleada = 100;
	short limite_Tiempo_Oleada = 20;
	short puntos;
};

struct Imagenes {
	void* plantas_dia;
	void* chomper_anim;
	void* semillero_bitmap;
	void* semillero_recarga;
	void* pala_cursor;
	void* pala_interfaz;
	void* sol_bitmap;
	void* explosion;
	void* guisante;
	void* zombie_bitmap;
	void* fondo_casa_dia;
};

struct Particula {
	short id;
	short x, y;
	short mov_x, impulso;
	short angulo;
	short estado;
	short tiemp;
	Particula *ant_part, *sig_part;
}	static* particulas;

/*----------PROTOTIPOS----------------------------------------------------------------------------------------------------*/

//INPUT
void registrar_teclas(ALLEGRO_EVENT);
void registrar_mouse(ALLEGRO_EVENT);

//PLANTAS
void eliminar_planta(Planta&);
void funcion_planta(short, short);
void generar_planta(short, Planta&);
void plantar_planta(ALLEGRO_EVENT);
void seleccionar_planta(ALLEGRO_EVENT, short);
short animacion_planta(short, short, bool);

//SOL
bool funcion_sol(short, short, Sol&, short);
void generar_sol_recolect(short, short, short);
float animacion_sol(Sol&);

//PROYECTILES
bool funcion_proyectil(Proyectil&, short);
void generar_proyectil(Proyectil*, short, short, short);
bool mover_proyectil(Proyectil&, short);

//ZOMBIES
bool funcion_zombie(Zombie&, short);
void generar_zombie(short,short);
void mover_zombie(Zombie&, short);
short animacion_zombie(Zombie&);

//PARTICULAS
bool funcion_particula(Particula&);
void generar_particula(short, short, short);
void rotacion_particula_x(Particula&);
float animacion_cabeza_part(Particula&);
void oleadas_zombie();

//DIBUJADO
void dibujar_tablero(Imagenes, short, short);

//EXTRA
void funcion_semillero();
void dibujar_numero(short, float, float, ALLEGRO_COLOR);

/*----------FUNCIÓN-MAIN--------------------------------------------------------------------------------------------------*/

int main() {
	srand(time(0));
	Imagenes bitmap;
	short mouse_x{}, mouse_y{};
	std::cout << "INICIO DE PROGRAMA" << std::endl;

	//Inicializar soles
	sol_tablero = new Sol;
	sol_tablero->ant_sol = sol_tablero;
	sol_tablero->sig_sol = NULL;
	sol_tablero->estado_act = 4;	
	
	//Inicializar partículas
	particulas = new Particula;
	particulas->ant_part = particulas;
	particulas->sig_part = NULL;
	particulas->id = -1;

	//Inicializar Proyectiles
	for (int i{}; i < CAS_Y; i++) {
		proyectil[i] = new Proyectil;
		proyectil[i]->ant_proy = proyectil[i];
		proyectil[i]->sig_proy = NULL;
		cant_proy[i] = 0;
	}
	//Incializar Zombies
	for (int i{}; i < CAS_Y; i++) {
		zombie[i] = new Zombie;
		zombie[i]->id = -1;
		zombie[i]->ant_zomb = zombie[i];
		zombie[i]->sig_zomb = NULL;
		cant_zombi[i] = 0;
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

	std::cout << "EMPEZAR EN PANTALLA COMPLETA? V/F" << std::endl;
	char opc;
	std::cin >> opc;
	switch (opc) {
	case 'v':al_set_new_display_flags(ALLEGRO_FULLSCREEN); break;
	}
	ALLEGRO_DISPLAY* display = al_create_display(RESOL_X, RESOL_Y);
	ALLEGRO_EVENT_QUEUE* cola_eventos = al_create_event_queue();
	ALLEGRO_TIMER* tiempo = al_create_timer(1.0 / 60);
	ALLEGRO_EVENT eventos;
	ALLEGRO_COLOR color_aux = NORM_C;
	ALLEGRO_BITMAP
		* plantas_dia_bitmap = al_load_bitmap("Sprites/Plants/Plants_Daytime.png"),
		* chomper_anim_bitmap = al_load_bitmap("Sprites/Plants/Chomper_Bite.png"),
		* semillero_bitmap = al_load_bitmap("Sprites/Extra/Seedpackets_Bitmap.png"),
		* semillero_recarga = al_load_bitmap("Sprites/Extra/Delay_Seedpacket.png"),
		* pala_cursor = al_load_bitmap("Sprites/Plants/Shovel.png"),
		* pala_interfaz = al_load_bitmap("Sprites/Extra/Shovel_Hud.png"),
		* sol_bitmap = al_load_bitmap("Sprites/Extra/Sun_Bitmap.png"),
		* explosion = al_load_bitmap("Sprites/Extra/Explosion_Red.png"),
		* guisante = al_load_bitmap("Sprites/Bullets/Bullet_Pea.png"),
		* zombie_bitmap = al_load_bitmap("Sprites/Zombies/Zombie_Basic.png"),
		* fondo_casa_dia = al_load_bitmap("Sprites/Daylight_Playground.png");

	bitmap.plantas_dia = plantas_dia_bitmap;
	bitmap.chomper_anim = chomper_anim_bitmap;
	bitmap.semillero_bitmap = semillero_bitmap;
	bitmap.semillero_recarga = semillero_recarga;
	bitmap.pala_cursor = pala_cursor;
	bitmap.pala_interfaz = pala_interfaz;
	bitmap.sol_bitmap = sol_bitmap;
	bitmap.explosion = explosion;
	bitmap.guisante = guisante;
	bitmap.zombie_bitmap = zombie_bitmap;
	bitmap.fondo_casa_dia = fondo_casa_dia;

	al_set_window_title(display, "Plantas Contra Zombies Remake");

	//Registrar entradas. Teclado, mouse y tiempo
	al_register_event_source(cola_eventos, al_get_keyboard_event_source());
	al_register_event_source(cola_eventos, al_get_mouse_event_source());
	al_register_event_source(cola_eventos, al_get_timer_event_source(tiempo));

	//Iniciar el tiempo
	al_start_timer(tiempo);

	while (bucle) {
		short sobre_dibujo{ 0 };
		al_wait_for_event(cola_eventos, &eventos);
		//Registrar posición del mouse
		if (eventos.type == ALLEGRO_EVENT_MOUSE_AXES) {
			mouse_x = eventos.mouse.x;
			mouse_y = eventos.mouse.y;
		}
		switch (eventos.type) {
		//El juego avanza
		case ALLEGRO_EVENT_TIMER:
			//Revisar funciones
			if (!(frames % 6)) {
				funcion_semillero();
				oleadas_zombie();
				//Revisar soles
				if (cant_sol_tablero) {
					Sol* ptr_sol = NULL, * anterior_sol = NULL;
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
				for (int i{}; i < CAS_Y; i++) {
					for (int j{}; j < CAS_X; j++) {
						funcion_planta(j, i);
					}
					if (cant_zombi[i]) {
						Zombie* ptr_zomb = NULL, * anterior_zomb = NULL;
						ptr_zomb = zombie[i];
						for (int j{}; j < cant_zombi[i]; j++) {
							anterior_zomb = ptr_zomb;
							ptr_zomb = ptr_zomb->sig_zomb;
							if (funcion_zombie(*ptr_zomb, i)) {
								ptr_zomb = anterior_zomb;
								j--;
							}
						}
					}
					if (cant_proy[i]) {
						Proyectil* ptr_proy = NULL, * anterior_proy = NULL;
						ptr_proy = proyectil[i];
						for (int j{}; j < cant_proy[i]; j++) {
							anterior_proy = ptr_proy;
							ptr_proy = ptr_proy->sig_proy;
							if (funcion_proyectil(*ptr_proy, i)) {
								ptr_proy = anterior_proy;
								j--;
							}
						}
					}
				}
			}
			if (!(frames % 2400)) {
				generar_sol_recolect(rand() % (RESOL_X - 500) + 300, 0, 25);
			}

			dibujar_tablero(bitmap, mouse_x, mouse_y);
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
	al_destroy_bitmap(chomper_anim_bitmap);
	al_destroy_bitmap(plantas_dia_bitmap);
	al_destroy_bitmap(semillero_bitmap);
	al_destroy_bitmap(pala_interfaz);
	al_destroy_bitmap(pala_cursor);
	al_destroy_bitmap(sol_bitmap);
	al_destroy_bitmap(semillero_recarga);
	al_destroy_bitmap(explosion);
	al_destroy_bitmap(guisante);
	al_destroy_bitmap(zombie_bitmap);

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
	
	std::cout << "ELIMINACION DE PARTÍCULAS:" << std::endl;
	if (cant_particulas) {
		Particula* elim_part = NULL, * sig_elim_part = NULL;
		elim_part = particulas;
		for (int i{}; i < cant_particulas; i++) {
			sig_elim_part = elim_part->sig_part;
			delete elim_part;
			std::cout << "POSICION " << i << " ELIMINADA" << std::endl;
			elim_part = sig_elim_part;
		}
	}
	else {
		std::cout << "PARTICULAS ELIMINADO" << std::endl;
		delete particulas;
	}

	std::cout << "ELIMINACION DE GUISANTES EN LA PANTALLA DEL JUEGO" << std::endl;
	for (int i{}; i < CAS_Y; i++) {
		if (cant_proy[i]) {
			Proyectil* elim_proy = NULL, * sig_elim_proy = NULL;
			elim_proy = proyectil[i];
			for (int j{}; j < cant_proy[i]; j++) {
				sig_elim_proy = elim_proy->sig_proy;
				delete elim_proy;
				std::cout << "POSICION " << j << " DE COL" << i << " ELIMINADA" << std::endl;
				elim_proy = sig_elim_proy;
			}
		}else{
			std::cout << "PROYECTIL COL" << i << " ELIMINADO" << std::endl;
			delete proyectil[i];
		}
	}

	std::cout << "ELIMINACION DE ZOMBIES EN LA PANTALLA DEL JUEGO" << std::endl;
	for (int i{}; i < CAS_Y; i++) {
		if (cant_zombi[i]) {
			Zombie* elim_zom = NULL, * sig_elim_zom = NULL;
			elim_zom = zombie[i];
			for (int j{}; j < cant_zombi[i]; j++) {
				sig_elim_zom = elim_zom->sig_zomb;
				delete elim_zom;
				std::cout << "POSICION " << j << " DE COL" << i << " ELIMINADA" << std::endl;
				elim_zom = sig_elim_zom;
			}
		}
		else {
			std::cout << "ZOMBIE COL" << i << " ELIMINADO" << std::endl;
			delete zombie[i];
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
	case ALLEGRO_KEY_Z: generar_zombie(rand() % CAS_Y, rand()%3); break;
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

void funcion_planta(short x,short y) {
	if (planta[x][y].pos) {
		if (planta[x][y].pv <= 0) {
			eliminar_planta(planta[x][y]);
			return;
		}
		switch (planta[x][y].pos) {
		case 1://LANZAGUISANTES
			if (planta[x][y].estado == 0 && cant_zombi[y] > 0 && planta[x][y].tiemp >= 12 + rand() % 4) {
				planta[x][y].tiemp = -1;
				planta[x][y].estado = 1;
				planta[x][y].animacion = 2 * LIM_F_PLANT - LIM_F_PLANT / 4 * 3;
			}
			if (planta[x][y].estado == 1) {
				if (planta[x][y].tiemp >= 3) {
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
				if (planta[x][y].tiemp >= 15) {
					planta[x][y].animacion = rand() % 100;
					planta[x][y].tiemp = 0;
					planta[x][y].estado = 0;
					generar_sol_recolect(x * 100 + 195, y * 100 + 165, 25);
				}
			}
			else if (planta[x][y].estado == 0) {
				//cambio de estado: soles
				if (planta[x][y].tiemp >= 135 + rand() % 20) {
					planta[x][y].animacion = -1;
					planta[x][y].tiemp = 0;
					planta[x][y].estado = 1;
					std::cout << "SOL GENERANDO" << std::endl;
				}
			}
			break;
		case 3://PETACEREZA
			switch (planta[x][y].estado) {
			case 0://se infla
				if (planta[x][y].animacion >= 3 * LIM_F_PLANT * 2 - 1) {
					planta[x][y].tiemp = 0;
					planta[x][y].animacion = 0;
					planta[x][y].estado = 1;
					anim_sobre_tablero++;
				}
				break;
			case 1://Evita hacer daño doble
				planta[x][y].estado = 2;
				break;
			case 2://Termina de explotar
				if (planta[x][y].animacion >= 12 * LIM_F_EXPLO) {
					anim_sobre_tablero--;
					eliminar_planta(planta[x][y]);
				}
				break;
			}
		case 5://PAPAPUM
			switch (planta[x][y].estado) {
			case 0://Se arma
				if (planta[x][y].tiemp >= 200) {
				planta[x][y].animacion = 0;
				planta[x][y].tiemp = 0;
				planta[x][y].estado = 1;
				std::cout << "PAPAPUM ARMADA" << std::endl;
				}
				break;
			case 2://Explota
				if (planta[x][y].tiemp >= 10) {
					planta[x][y].estado = 3;
					planta[x][y].tiemp = 0;
					planta[x][y].animacion = 0;
					anim_sobre_tablero++;
				}
				break;
			case 3://Ya no hace daño acumulado
				planta[x][y].estado = 4;
				break;
			case 4://Explota animación
				if (planta[x][y].tiemp >= 20) {
					anim_sobre_tablero--;
					eliminar_planta(planta[x][y]);
				}
				break;
			}
			break;
		case 6://Hielaguisantes
			if (planta[x][y].estado == 0 && cant_zombi[y] > 0 && planta[x][y].tiemp >= 12 + rand() % 5) {
				planta[x][y].tiemp = -1;
				planta[x][y].estado = 1;
				planta[x][y].animacion = 2 * LIM_F_PLANT - LIM_F_PLANT / 4 * 3;
			}
			if (planta[x][y].estado == 1) {
				if (planta[x][y].tiemp >= 3) {
					generar_proyectil(proyectil[y], y, planta[x][y].pos, x);
					planta[x][y].tiemp = -1;
					planta[x][y].animacion = 2 * LIM_F_PLANT;
					planta[x][y].estado = 0;
				}
			}
			break;
		case 7://Carroñivora
			if (planta[x][y].estado == 1) {
				if (planta[x][y].tiemp >= 250) {
					planta[x][y].estado = 0;
					anim_sobre_tablero--;
				}
			}
			break;
		case 8://Repetidora
			if (planta[x][y].estado == 0 && cant_zombi[y] > 0 && planta[x][y].tiemp >= 12 + rand() % 5) {
				planta[x][y].tiemp = -1;
				planta[x][y].estado = 1;
				planta[x][y].tiemp_evento = 0;
				planta[x][y].animacion = 2 * LIM_F_PLANT - LIM_F_PLANT / 4 * 3;
			}
			if (planta[x][y].estado == 1) {
				if (planta[x][y].tiemp >= 3) {
					generar_proyectil(proyectil[y], y, planta[x][y].pos, x);
					planta[x][y].tiemp = 0;
					planta[x][y].animacion = 2 * LIM_F_PLANT - LIM_F_PLANT / 4 * 3;
					planta[x][y].tiemp_evento++;
				}
				if (planta[x][y].tiemp_evento >= 2) {
					planta[x][y].animacion = 2 * LIM_F_PLANT;
					planta[x][y].tiemp = -1;
					planta[x][y].estado = 0;
				}
			}
			break;
		}
		planta[x][y].tiemp++;
	}
}

void generar_planta(short id_planta, Planta& planta) {
	planta.pv = PV_PLANTA[id_planta];
	planta.pos = id_planta;
	planta.tiemp = 0;
	planta.estado = 0;
	planta.tiemp_evento = 0;
	planta.animacion = 0;
	planta.anim_danio = 0;
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
				soles_guard_suma -= COST_PLANTA[planta_elegida];
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

short animacion_planta(short x, short y, bool corte_vert) {
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
			else if (planta[x][y].animacion == 2 && planta[x][y].tiemp >= 12) planta[x][y].animacion++;
			return planta[x][y].animacion * 90 + 450;
		}
		break;
	case 3://PETACEREZA
		switch (planta[x][y].estado) {
		case 0:
			if (planta[x][y].animacion < 3 * (LIM_F_PLANT * 2) - 1) planta[x][y].animacion++;
			return (int(planta[x][y].animacion / (LIM_F_PLANT * 2)) * 90);
		default:
			if (planta[x][y].animacion < 12 * LIM_F_EXPLO) planta[x][y].animacion++;
			return -90;
		}
		break;
	case 4://NUEZ
		if (planta[x][y].pv > 170) {
			return 0 + (planta[x][y].tiemp % 20 ? 0 : 1) * 360;
		}
		else if (planta[x][y].pv > 100 && planta[x][y].pv <= 170) {
			return 90 + (planta[x][y].tiemp % 18 ? 0 : 1) * 360;
		}
		else if (planta[x][y].pv > 40 && planta[x][y].pv <= 100) {
			return 180 + (planta[x][y].tiemp % 15 ? 0 : 1) * 360;
		}
		else {
			return 270 + (planta[x][y].tiemp % 8 ? 0 : 1) * 360;
		}
		break;
	case 5://PAPAPUM
		switch (planta[x][y].estado) {
		case 0:
			//NORMAL
			return ((planta[x][y].tiemp >= 162 ? planta[x][y].tiemp / 2 : 0) % 2) * 90 + 180;
		case 1:
			//ACTIVA
			planta[x][y].animacion++;
			switch (int(planta[x][y].animacion / 8)) {
			case 0:
				return 360 + ((planta[x][y].tiemp / 2) % 2 * 270);
			case 1:
				return 450 + ((planta[x][y].tiemp / 2) % 2 * 270);
			case 3: case 4:
				return 540 + ((planta[x][y].tiemp / 2) % 2 * 270);
			default:
				return ((planta[x][y].tiemp / 2) % 2) * 90 + (planta[x][y].tiemp % 22 ? 0 : 1) * 900;
			}
		default:
			if (planta[x][y].animacion < 12 * LIM_F_EXPLO) planta[x][y].animacion++;
			return -90;
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
		case 1://ATAQUE
			if (corte_vert) {
				if (planta[x][y].animacion < 4 * LIM_F_PLANT - 2) planta[x][y].animacion++;
				return int(planta[x][y].animacion / LIM_F_PLANT) * 180;
				break;
			}
			else {
				return -90;
			}
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
	//Es cualquier estado excepto el 3
	if (sol.estado_act != 3) {
		//Encontrar la posición vertical del sol
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
			if (y >= sol.estado.cayendo.mov_y) {
				sol.estado_act = 2;
				sol.tiemp = 0;
			}
		default:
			x = sol.x;
			y = sol.y;
			sol.tiemp++;
			break;
		}
		//revisar si el mouse está en la zona de recolección del sol
		if (mouse_x > x - 160 && mouse_x < x + 160 && mouse_y > y - 160 && mouse_y < y + 160) {
			std::cout << "SOL RECOLECTADO" << std::endl;
			soles_guard += sol.cant;
			soles_guard_suma += sol.cant;
			sol.x = x;
			sol.y = y;
			sol.estado.recol.mov_x = x - 40;
			sol.estado.recol.mov_y = y - 10;
			sol.estado_act = 3;
		}
		//El sol ya expiró. Desaparece
		if (sol.estado_act == 2 && sol.tiemp >= 150) {
			goto Eliminar_sol;
		}
	}
	//Se recolectó el sol
	else {
		//Si el sol ya entró en la zona de la interfaz
		if (sol.x <= 50 || sol.y <= 25) {
		Eliminar_sol:
			//Eliminar espacio reservado en la memoria para el sol
			// Revisar si se apunta a NULL
			if (sol.sig_sol) {
				//No se apunta NULL
				//Conectar el anterior y el siguiente sol entre sí
				Sol* anterior_sol = NULL, * siguiente_sol = NULL;

				siguiente_sol = sol.sig_sol;
				std::cout << "ANTERIOR APUNTADO: " << sol.sig_sol << std::endl;
				std::cout << "NUEVO ANT APUNTADO: " << sol.ant_sol->sig_sol << std::endl;
				sol.ant_sol->sig_sol = siguiente_sol;

				anterior_sol = sol.ant_sol;
				std::cout << "SIGUIENTE APUNTADO: " << sol.ant_sol << std::endl;
				std::cout << "NUEVO SIG APUNTADO: " << sol.sig_sol->ant_sol << std::endl;
				sol.sig_sol->ant_sol = anterior_sol;
			}
			else {
				//Sí apunta a NULL
				//Anterior sol apunta a NULL
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
	short direccion = sol.estado.anim.d ? 1: -1;
	//Determinar dirección a la que se dirigirá el sol
	if (direccion == 1) {
		//Derecha
		//Se traslada hacia la derecha
		if (sol.estado.anim.mov_x < sol.estado.anim.impulso * 2) {
			sol.estado.anim.mov_x += sol.estado.anim.impulso / (sol.estado.anim.impulso / 6 * 3);
		}
		//Se detiene al llegar al final de la función matemática
		if (sol.estado.anim.mov_x >= sol.estado.anim.impulso * 2) {
			sol.estado.anim.mov_x = sol.estado.anim.impulso * 2;
			sol.estado_act = 1;
		}
	}
	else if (direccion == -1) {
		//Izquierda
		//Se traslada hacia la izquierda
		if (sol.estado.anim.mov_x > sol.estado.anim.impulso * -2) {
			sol.estado.anim.mov_x -= sol.estado.anim.impulso / (sol.estado.anim.impulso / 6 * 3);
		}
		//Se detiene al llegar al final de la función matemática
		if (sol.estado.anim.mov_x <= sol.estado.anim.impulso * -2) {
			sol.estado.anim.mov_x = sol.estado.anim.impulso * -2;
			sol.estado_act = 1;
		}
	}
	//Retornar extra en y
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
	nuevo_sol->angulo = rand() % 720 - 360;
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

bool funcion_proyectil(Proyectil &proy, short fila) {
	Zombie *zomb_atacado = NULL, *zomb_busqueda = NULL;
	zomb_busqueda = zombie[fila];
	//Busca un zombie a atacar
	for (int i{}; i < cant_zombi[fila]; i++) {
		zomb_busqueda = zomb_busqueda->sig_zomb;
		//El guisante está en el rango de un zombie
		if (proy.x >= zomb_busqueda->x && proy.x < zomb_busqueda->x + 108) {
			//Si no hay un zombie a atacar, asigna uno nuevo
			if (zomb_busqueda->pv > 0 && zomb_busqueda->est_danio >= 0) {
				if (!zomb_atacado) {
					zomb_atacado = zomb_busqueda;
				}
				//Hay un zombie a atacar
				else {
					//Decide cuál está más cerca del guisante
					if (proy.x - zomb_atacado->x < proy.x - zomb_busqueda->x) {
						//el nuevo zombie encontrado está más cerca
						zomb_atacado = zomb_busqueda;
					}
					//Las posiciones son iguales
					else if (proy.x - zomb_atacado->x == proy.x - zomb_busqueda->x) {
						//Revisa cuál tiene mayor prioridad
						if (zomb_atacado->prior > zomb_busqueda->prior) {
							zomb_atacado = zomb_busqueda;
						}
					}
				}
			}
		}
	}
	if (zomb_atacado) {
		switch (proy.tipo) {
		case 0://Guisante normal
			zomb_atacado->pv -= 1;
			break;
		case 1://Guisante congelado WIP
			zomb_atacado->pv -= 1;
			zomb_atacado->estado = 1;
			zomb_atacado->tiemp = 60;
			break;
		}
		if (zomb_atacado->anim_danio < 18) {
			zomb_atacado->anim_danio = 24;
		}

		//borra el proyectil
		if (proy.sig_proy) {
			proy.ant_proy->sig_proy = proy.sig_proy;
			proy.sig_proy->ant_proy = proy.ant_proy;
		}
		else {
			proy.ant_proy->sig_proy = NULL;
		}
		cant_proy[fila]--;
		std::cout << "PROYECTIL ELIMINADO: " << &proy << std::endl;
		delete& proy;
		return 1;
	}
	return 0;
}

void generar_proyectil(Proyectil *proy, short y, short tipo, short pos_x) {
	Proyectil* nuevo_proy = NULL, * anterior_proy = NULL;
	nuevo_proy = anterior_proy = proy;
	for (int i{}; i < cant_proy[y]; i++) {
		nuevo_proy = nuevo_proy->sig_proy;
	}
	//Generar nuevo espacio en memoria
	nuevo_proy->sig_proy = new Proyectil;
	//asignar nuevo espacio en memoria
	anterior_proy = nuevo_proy;
	nuevo_proy= nuevo_proy->sig_proy;

	//Asignar punteros
	nuevo_proy->ant_proy = anterior_proy;
	nuevo_proy->sig_proy = NULL;

	nuevo_proy->x = pos_x * 100 + 270;
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

/*----------FUNCIONES-ZOMBIE----------------------------------------------------------------------------------------------*/

bool funcion_zombie(Zombie& zomb, short fila) {
	if (zomb.pv > 0) {
		short casilla = -1;
		int sx = 0;
		switch (zomb.id) {
		case 1://Caracono
			if (zomb.pv <= 17 && zomb.est_danio == 2) {
				zomb.est_danio = 3;
			}
			else if (zomb.pv <= 10 && zomb.est_danio == 3) {
				zomb.est_danio = 0;
				generar_particula(zomb.x, fila * 100 + 120, 1);
			}
			break;
		case 2://Cubeta
			if (zomb.pv <= 23 && zomb.est_danio == 2) {
				zomb.est_danio = 3;
			}
			else if (zomb.pv <= 10 && zomb.est_danio == 3) {
				zomb.est_danio = 0;
				generar_particula(zomb.x, fila * 100 + 120, 2);
			}
			break;
		}
		if (zomb.pv < 5 && zomb.est_danio == 0) {
			zomb.est_danio = 1;
			generar_particula(zomb.x, fila * 100 + 120, 5);
		}

		for (int i{}; i <= CAS_X + 1; i++) {
			if (zomb.x >= 150 + i * 100 && zomb.x < 250 + i * 100) {
				casilla = i;
			}
		}
		if (zomb.estado == 1 && zomb.tiemp > 0) {
			zomb.tiemp--;
		}
		else {
			zomb.estado = 0;
		}
		if (casilla >= 0) {
			if (casilla < CAS_X) {
				//busca si se encuentra con una planta
				if (planta[casilla][fila].pos) {
					if (!(zomb.estado == 1 && frames % 20)) {
						switch (planta[casilla][fila].pos) {
						case 3://Petacereza
							break;
						case 5:
							if (planta[casilla][fila].estado) {
								break;
							}

						default:
							planta[casilla][fila].pv--;
							if (!planta[casilla][fila].anim_danio) {
								planta[casilla][fila].anim_danio = 24;
							}
							zomb.comiendo = true;
							break;
						}
					}
				}
				else {
					zomb.comiendo = false;
				}
			}

			if (anim_sobre_tablero) {
				for (int y{ -1 }; y <= 1; y++) {
					if (fila + y >= 0 && fila + y < CAS_Y) {
						for (int x{ -1 }; x <= 1; x++) {
							if (casilla + x >= 0 && casilla + x < CAS_X) {
								if ((planta[casilla + x][fila + y].pos == 3 && planta[casilla + x][fila + y].estado == 1)) {
									zomb.pv -= 100;
									if (zomb.pv <= 0) {
										goto eliminar_zombie;
									}
								}
							}
						}
					}
				}
				if (planta[casilla][fila].pos == 5) {
					switch (planta[casilla][fila].estado) {
					case 1:
						planta[casilla][fila].estado = 2;
						planta[casilla][fila].tiemp = 0;
						break;
					case 3:
						zomb.pv -= 100;
						if (zomb.pv <= 0) {
							goto eliminar_zombie;
						}
						break;
					}
				}

				//Revisar casilla y media más adelante
				if ((int(zomb.x) - 150) % 100 <= 50) {
					sx = -2;
				}
				else {
					sx = -1;
				}
				for (sx; sx <= 0; sx++) {
					//Encuentra planta carroñivora
					if (planta[casilla + sx][fila].pos == 7 && casilla + sx >= 0) {
						switch (planta[casilla + sx][fila].estado){
						case 0://Carroñivora muerde
							planta[casilla + sx][fila].estado = 1;
							planta[casilla + sx][fila].tiemp = 0;
							planta[casilla + sx][fila].animacion = 0;
							anim_sobre_tablero++;
							break;
						case 1://Carroñivora hace daño
							if (int(planta[casilla + sx][fila].animacion / LIM_F_PLANT) == 2) {
								if (zomb.est_danio >= 2)
								switch (zomb.id) {
								case 1: generar_particula(zomb.x, fila * 100 + 120, zomb.est_danio == 2 ? 3 : 1); break;
								case 2: generar_particula(zomb.x, fila * 100 + 120, zomb.est_danio == 2 ? 4 : 2); break;
								}
								generar_particula(zomb.x, fila * 100 + 120, 0);
								goto eliminar_zombie;
							}
						default:
							break;
						}
					}
				}
			}
		}
	}
	else {
		zomb.tiemp++;
		if (zomb.est_danio != -1) {
			generar_particula(zomb.x, fila * 100 + 120, 0);
			zomb.tiemp = 0;
			zomb.est_danio = -1;
		}
		if (zomb.tiemp >= 24 && zomb.est_danio == -1) {
		eliminar_zombie:
			if (zomb.sig_zomb) {
				zomb.ant_zomb->sig_zomb = zomb.sig_zomb;
				zomb.sig_zomb->ant_zomb = zomb.ant_zomb;
			}
			else {
				zomb.ant_zomb->sig_zomb = NULL;
			}
			cant_zombi[fila]--;
			std::cout << "ZOMBIE MORIDO: " << &zomb << std::endl;
			delete& zomb;
			return 1;
		}
	}
	return 0;
}

void mover_zombie(Zombie& zomb, short fila) {
	if (!zomb.comiendo) {
		if (zomb.x < 155) {
			//FIN DEL JUEGO WIP
		}
		else {
			switch (zomb.estado) {
			case 1:
				zomb.x -= .11;
				break;
			default:
				zomb.x -= .22;
				break;
			}
		}
	}
}

void generar_zombie(short y ,short tipo) {
	Zombie* nuevo_zomb = NULL, * anterior_zomb = NULL, * ptr_zomb = NULL;
	nuevo_zomb = anterior_zomb = zombie[y];

	//Encontrar última posición
	for (int i{}; i < cant_zombi[y]; i++) {
		ptr_zomb = nuevo_zomb->sig_zomb;
		nuevo_zomb = ptr_zomb;
	}

	//Generar nuevo espacio en memoria
	nuevo_zomb->sig_zomb = new Zombie;
	//asignar nuevo espacio en memoria
	anterior_zomb = nuevo_zomb;
	nuevo_zomb = nuevo_zomb->sig_zomb;

	//Asignar punteros
	nuevo_zomb->ant_zomb = anterior_zomb;
	nuevo_zomb->sig_zomb = NULL;

	//Inicializar al zombie
	nuevo_zomb->x = RESOL_X;
	nuevo_zomb->id = tipo;

	switch (nuevo_zomb->id) {
	default://Zombie común y por defecto
		nuevo_zomb->pv = 10;
		nuevo_zomb->est_danio = 0;
		break;	
	case 1://Zombie cara cono
		nuevo_zomb->pv = 28;
		nuevo_zomb->est_danio = 2;
		break;	
	case 2://Zombie cara cubo
		nuevo_zomb->pv = 65;
		nuevo_zomb->est_danio = 2;
		break;	
	}

	nuevo_zomb->animacion = 0;
	nuevo_zomb->prior = 0;
	nuevo_zomb->tiemp = 0;
	nuevo_zomb->anim_danio = 0;
	nuevo_zomb->comiendo = false;

	cant_zombi[y]++;
}

void oleadas_zombie() {
}

short animacion_zombie(Zombie& zomb) {
	switch (zomb.id) {
	case 0: case 1: case 2:
		if (zomb.estado == 1 && frames % 2) {
			zomb.animacion--;
		}
		if (zomb.animacion < 12 * LIM_F_ZOMB - 1) zomb.animacion++;
		else if (zomb.animacion >= 12 * LIM_F_ZOMB - 1) zomb.animacion = 0;
		return int(zomb.animacion / LIM_F_ZOMB) * 108;
		break;
	}
	return 0;
}

/*----------FUNCIONES-PARTÍCULAS------------------------------------------------------------------------------------------*/

bool funcion_particula(Particula& part) {
	bool eliminar{ false };
	if (part.tiemp > 2000) {
		eliminar = true;
	}
	//Determinar tipo de animación a realizar
	switch (part.estado) {
		//Case 0: Vuela la cabeza
	case 1://Transición a movimiento
		part.y = animacion_cabeza_part(part);
		part.estado = 2;
		part.tiemp = 0;
		break;
		//Case 2: Rueda hacia la derecha
	case 3://Termina el movimiento
		if (part.tiemp >= 600) {
			part.estado = 4;
			part.tiemp = 0;
		}
		break;
	case 4://Se desvanece
		if (part.tiemp > 250) {
			eliminar = true;
		}
	}
	part.tiemp++;
	if (eliminar) {
		if (part.sig_part) {
			part.ant_part->sig_part = part.sig_part;
			part.sig_part->ant_part = part.ant_part;
		}
		else {
			part.ant_part->sig_part = NULL;
		}
		cant_particulas--;
		std::cout << "Particula final: " << &part << std::endl;
		delete& part;
		return 1;
	}
}

void generar_particula(short x, short y, short id) {
	Particula* nueva_part = NULL, * ant_part = NULL;
	nueva_part = ant_part = particulas;
	for (int i{}; i < cant_particulas; i++) {
		nueva_part = nueva_part->sig_part;
	}
	std::cout << "Y: " << y << std::endl;
	//Generar nuevo espacio en memoria
	nueva_part->sig_part = new Particula;
	//asignar nuevo espacio en memoria
	ant_part = nueva_part;
	nueva_part = nueva_part->sig_part;

	//Asignar punteros
	nueva_part->ant_part = ant_part;
	nueva_part->sig_part = NULL;

	std::cout << "PARTICULA NUEVA: " << nueva_part << std::endl;

	//Inicializar nueva partícula
	nueva_part->id = id;
	switch (id) {
	case 0://Cabeza
		nueva_part->estado = 0;
		nueva_part->x = x + 25;
		nueva_part->mov_x = 0;
		nueva_part->impulso = rand() % 24 + 20;
		nueva_part->angulo = 0;
		nueva_part->tiemp = 0;
		nueva_part->y = y;
		break;
	case 1: case 3://Cono dañado y sin dañar
		nueva_part->estado = 0;
		nueva_part->x = x + 23;
		nueva_part->mov_x = 0;
		nueva_part->impulso = rand() % 20 + 20;
		nueva_part->angulo = 0;
		nueva_part->tiemp = 0;
		nueva_part->y = y - 30;
		break;
	case 2: case 4://Cubo dañado y sin dañar
		nueva_part->estado = 0;
		nueva_part->x = x + 15;
		nueva_part->mov_x = 0;
		nueva_part->impulso = rand() % 18 + 20;
		nueva_part->angulo = 0;
		nueva_part->tiemp = 0;
		nueva_part->y = y - 15;
		break;
	case 5://brazo
		nueva_part->estado = 0;
		nueva_part->x = x + 56;
		nueva_part->mov_x = 0;
		nueva_part->impulso = rand() % 18 + 20;
		nueva_part->angulo = -90;
		nueva_part->tiemp = 0;
		nueva_part->y = y + 34;
		break;
	}
	cant_particulas++;
}

void rotacion_particula_x(Particula& part) {
	switch (part.id) {
	case 0:
		part.x ++;
		part.angulo += part.impulso / 4;
		break;
	case 1: case 3:
		part.x ++;
		part.angulo += part.impulso / 6;
		break;
	case 2: case 4:
		part.x++;
		part.angulo += part.impulso / 8;
		part.impulso--;
		break;
	case 5:
		part.x++;
		part.angulo += part.impulso / 12;
		part.impulso--;
		break;
	}
	part.impulso--;
	if (part.impulso <= 0) {
		part.estado = 3;
		part.tiemp = 0;
	}
}

float animacion_cabeza_part(Particula& part) {
	float mult{ 1.5 }, caida{ 1 };
	if (!part.estado) {
		//La partícula es una cabeza
		if (part.id != 5) {
			part.angulo += (part.impulso - 18) / 3;
		}
		//La partícula es un brazo
		else {
			part.angulo += (part.impulso - 8) / 5;
		}
	}
	//Derecha
	//Se traslada hacia la derecha
	if (part.mov_x < part.impulso * 2) {
		part.mov_x += part.impulso / (part.impulso / 6 * 3);
	}
	//Se detiene al llegar al final de la función matemática
	if (part.mov_x >= part.impulso * 2) {
		part.mov_x = part.impulso * 2;
		part.estado = 1;
	}
	//Caso especial para la partícula brazo
	if (part.id == 5) {
		mult = .75;
		caida = 1;
	}
	//Retornar extra en y
	return part.y - sqrt(pow(float(part.impulso), 2) - pow(part.mov_x - part.impulso, 2)) * mult + part.mov_x * caida;
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

/*----------------------DIBUJADO------------------------------------------------------------------------------------------*/

void dibujar_tablero(Imagenes b, short mouse_x, short mouse_y) {
	ALLEGRO_COLOR color_aux;
	short sobre_dibujo{ 0 };

	al_draw_bitmap((ALLEGRO_BITMAP*)b.fondo_casa_dia, 0, 0, 0);

	//Dibujar partículas
	if (cant_particulas) {
		Particula* ptr_part{ NULL }, * ant_part{ NULL };
		ptr_part = particulas;
		for (int i{}; i < cant_particulas; i++) {
			float pos_y{ 0 };
			ant_part = ptr_part;
			ptr_part = ptr_part->sig_part;
			if (funcion_particula(*ptr_part)) {
				ptr_part = ant_part;
				i--;
				continue;
			}

			color_aux = NORM_C;

			//Revisar estado actual de la partícula
			switch (ptr_part->estado) {
			case 0: case 1://Sale volando
				pos_y = animacion_cabeza_part(*ptr_part);
				break;
			case 2://Rota en el piso
				rotacion_particula_x(*ptr_part);
				pos_y = ptr_part->y;
				break;
			case 3://se queda quieto
				pos_y = ptr_part->y;
				break;
			case 4://Desaparece
				pos_y = ptr_part->y;
				color_aux = al_map_rgba(255 - ptr_part->tiemp, 255 - ptr_part->tiemp, 255 - ptr_part->tiemp, 255 - ptr_part->tiemp);
				break;
			}

			//Revisar tipo de partícula a dibujar
			switch (ptr_part->id) {
			case 0://Cabeza de zombi
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, 25, 338, 46, 46, color_aux, 19, 27, ptr_part->x + ptr_part->mov_x + 23, pos_y + 23, 1, 1, ptr_part->angulo * .01745, 0);
				break;
			case 1: case 3://Cono
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, (1 - ptr_part->id / 2) * 62, 810, 62, 62, color_aux, 32, 32, ptr_part->x + ptr_part->mov_x + 32, pos_y + 32, 1, 1, ptr_part->angulo * .01745, 0);
				break;
			case 2: case 4://Cubo
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, (1 - ptr_part->id / 2) * 62 + 186, 810, 62, 62, color_aux, 32, 32, ptr_part->x + ptr_part->mov_x + 32, pos_y + 32, 1, 1, ptr_part->angulo * .01745, 0);
				break;
			case 5://Brazo
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, 128, 350, 58, 24, color_aux, 19, 27, ptr_part->x + ptr_part->mov_x + 29, pos_y + 12, 1, 1, ptr_part->angulo * .01745, 0);
				break;
			}
		}
	}

	//Dibujar plantas, proyectiles y zombies en el tablero
	for (int y{}; y < CAS_Y; y++) {
		Proyectil* ptr_proy = NULL, * ant_proy = NULL;
		Zombie* ptr_zomb = NULL;
		for (int x{}; x < CAS_X; x++) {
			if (planta[x][y].pos) {
				if (planta[x][y].pos >= 0 && planta[x][y].pos <= 8) {
					char color = planta[x][y].anim_danio > 0 ? 255 - planta[x][y].anim_danio * 7 : 255;
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.plantas_dia, al_map_rgb(255, color, color), (planta[x][y].pos - 1) * 90, animacion_planta(x, y, false), 90, 90, x * 100 + 195, y * 100 + 165, 0);
					if (planta[x][y].anim_danio > 0) {
						planta[x][y].anim_danio--;
					}
				}
			}
		}
		ptr_proy = proyectil[y];
		for (int i{}; i < cant_proy[y]; i++) {
			ant_proy = ptr_proy;
			ptr_proy = ptr_proy->sig_proy;
			if (mover_proyectil(*ptr_proy, y)) {
				ptr_proy = ant_proy;
				i--;
				continue;
			}
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.guisante, ptr_proy->tipo * 28, 0, 28, 28, ptr_proy->x, y * 100 + 189 + rand() % 3, 0);
		}
		ptr_zomb = zombie[y];
		for (int i{}; i < cant_zombi[y]; i++) {
			ptr_zomb = ptr_zomb->sig_zomb;
		}
		for (int i{}; i < cant_zombi[y]; i++) {
			short color{ 0 };
			short color_r{ 255 }, color_g{ 255 }, color_b{ 255 };
			short estado_zomb{ 0 };
			float gorro_anim{ 0 };

			if (ptr_zomb->anim_danio > 0) {
				color = ptr_zomb->anim_danio > 0 ? 255 - ptr_zomb->anim_danio * 10 : 255;
				if (ptr_zomb->estado != 1)
					color_g = color_b = color;
				else
					color_g = color_r = color;
			}
			//Está congelado, se pinta de azul
			if (ptr_zomb->estado == 1) {
				color_r -= 190;
				if (color_r < 0)
					color_r = 0;
				color_g -= 110;
				if (color_g < 0)
					color_g = 0;
			}

			color_aux = al_map_rgb(color_r, color_g, color_b);

			//Cambiar al sprite de daño
			if (ptr_zomb->est_danio == 1) {
				estado_zomb = 135;
			}
			mover_zombie(*ptr_zomb, y);
			if (ptr_zomb->est_danio >= 0) {
				al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, animacion_zombie(*ptr_zomb), estado_zomb, 108, 135, (int)ptr_zomb->x, y * 100 + 120, 0);
				al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, animacion_zombie(*ptr_zomb), 270, 108, 67, (int)ptr_zomb->x, y * 100 + 120, 0);
			}
			else {
				al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, animacion_zombie(*ptr_zomb), estado_zomb, 108, 135, (int)ptr_zomb->x, y * 100 + 120, 0);
			}

			//Dibujar gorrito
			if (ptr_zomb->pv > 10 && ptr_zomb->id != 0) {
				switch (ptr_zomb->id) {
				case 1://Cono
					gorro_anim = int(ptr_zomb->animacion / LIM_F_ZOMB) * .4;
					if (gorro_anim >= 3) {
						gorro_anim *= -1;
						gorro_anim += 5;
					}
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, (ptr_zomb->est_danio - 2) * 62, 810, 62, 62, (int)ptr_zomb->x + 23, y * 100 + gorro_anim * 2 + 90, 0);
					break;
				case 2://Cubeta
					gorro_anim = int(ptr_zomb->animacion / LIM_F_ZOMB) * .4;
					if (gorro_anim >= 3) {
						gorro_anim *= -1;
						gorro_anim += 5.5;
					}
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, (ptr_zomb->est_danio - 2) * 62 + 124, 810, 62, 62, (int)ptr_zomb->x + 15, y * 100 + gorro_anim * 3 + 105, 0);
					break;
				}
			}

			if (ptr_zomb->anim_danio > 0) {
				ptr_zomb->anim_danio--;
			}
			ptr_zomb = ptr_zomb->ant_zomb;
		}

		//Dibujar elementos sobre la pantalla
		for (int x{}; x < CAS_X && sobre_dibujo < anim_sobre_tablero; x++) {
			if (planta[x][y].pos) {
				if (planta[x][y].pos == 3 && planta[x][y].estado >= 1) {
					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.explosion, (int(planta[x][y].animacion / LIM_F_EXPLO)) * 300, 0, 300, 300, x * 100 + 95, y * 100 + 65, 0);
					sobre_dibujo++;
				}
				else if (planta[x][y].pos == 5 && planta[x][y].estado >= 3) {
					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.explosion, (int(planta[x][y].animacion / LIM_F_EXPLO)) * 300, 0, 300, 300, x * 100 + 95, y * 100 + 65, 0);
					sobre_dibujo++;
				}
				else if (planta[x][y].pos == 7 && planta[x][y].estado == 1) {
					char color = planta[x][y].anim_danio > 0 ? 255 - planta[x][y].anim_danio * 7 : 255;
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.chomper_anim, al_map_rgb(255, color, color), animacion_planta(x, y, true), 0, 180, 135, x * 100 + 195, y * 100 + 120, 0);
					sobre_dibujo++;
				}
			}
		}
	}

	//Dibujar soles en el tablero
	if (cant_sol_tablero > 0) {
		Sol* ptr_sol = NULL;
		ptr_sol = sol_tablero;
		for (int i{}; i < cant_sol_tablero; i++) {
			if (ptr_sol->sig_sol)
				ptr_sol = ptr_sol->sig_sol;
			else break;
			//Dibujar animación de salto
			if (ptr_sol->estado_act >= 0 && ptr_sol->estado_act <= 1) {
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 130, 0, 80, 80, TRANS_C, 40, 40, ptr_sol->x + ptr_sol->estado.anim.mov_x + 40, animacion_sol(*ptr_sol) + 40, 1, 1, (float)ptr_sol->angulo * .01745, 0);
				al_draw_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 210, 0, 80, 80, ptr_sol->x + ptr_sol->estado.anim.mov_x, animacion_sol(*ptr_sol), 0);
			}

			//Dibujar animación cayendo del cielo
			else if (ptr_sol->estado_act == -1) {
				if (ptr_sol->y < ptr_sol->estado.cayendo.mov_y)
					ptr_sol->y += ptr_sol->estado.cayendo.mov_y / 500;
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 130, 0, 80, 80, TRANS_C, 40, 40, ptr_sol->x + 40, ptr_sol->y + 40, 1, 1, (float)ptr_sol->angulo * .01745, 0);
				al_draw_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 210, 0, 80, 80, ptr_sol->x, ptr_sol->y, 0);
			}
			//Dibujar sol quieto
			else if (ptr_sol->estado_act != 3) {
				//Si el sol está a punto de expirar, se hace esta operación para reducir la transparencia del sol
				unsigned char color = ptr_sol->tiemp > 120 ? 255 - ((ptr_sol->tiemp - 120) * 8) : 255;
				float tamanio;
				//Se calcula el tamaño que tendrá el halo de luz del sol
				tamanio = (ptr_sol->tiemp + 8) % 24 >= 12 ? -(ptr_sol->tiemp + 8) % 12 - .0125 : (ptr_sol->tiemp + 8) % 12;
				tamanio *= .0125;
				//Si es negativo es que está decreciendo, caso contrario esta creciendo
				if (tamanio < 0)
					tamanio += .15;
				//Se le suma un extra
				tamanio += .9;
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 130, 0, 80, 80, al_map_rgba(color * .8, color * .8, color * .8, color * .8), 40, 40, ptr_sol->x + 40, ptr_sol->y + 40, tamanio, tamanio, (float)ptr_sol->angulo * .01745, 0);
				al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, al_map_rgba(color, color, color, color), 210, 0, 80, 80, ptr_sol->x, ptr_sol->y, 0);
			}
			//Dibujar estado recolectado (va al sol de la interfaz)
			else {
				float escala, tamanio;
				//Mover al sol
				if (ptr_sol->x > 40 && ptr_sol->y > 10) {
					//Desacelera más
					if (ptr_sol->x < ptr_sol->estado.recol.mov_x / 5 ||
						ptr_sol->y < ptr_sol->estado.recol.mov_y / 5) { //TERMINA CONDICIÓN

						ptr_sol->x -= ptr_sol->estado.recol.mov_x / 90;
						ptr_sol->y -= ptr_sol->estado.recol.mov_y / 90;
					}
					//Empieza a desacelerar
					else if (ptr_sol->x < ptr_sol->estado.recol.mov_x / 3 ||
						ptr_sol->y < ptr_sol->estado.recol.mov_y / 3) { //TERMINA CONDICIÓN

						ptr_sol->x -= ptr_sol->estado.recol.mov_x / 50;
						ptr_sol->y -= ptr_sol->estado.recol.mov_y / 50;
					}
					//Velocidad normal
					else {
						ptr_sol->x -= ptr_sol->estado.recol.mov_x / 30;
						ptr_sol->y -= ptr_sol->estado.recol.mov_y / 30;
					}
				}
				//Se calcula el tamaño que tendrá el halo de luz del sol
				tamanio = (ptr_sol->tiemp + 8) % 24 >= 12 ? -(ptr_sol->tiemp + 8) % 12 - .0125 : (ptr_sol->tiemp + 8) % 12;
				tamanio *= .0125;
				//Si es negativo es que está decreciendo, caso contrario esta creciendo
				if (tamanio < 0)
					tamanio += .15;
				escala = ((float)ptr_sol->x + (float)ptr_sol->y) / (ptr_sol->estado.recol.mov_x + ptr_sol->estado.recol.mov_y) - .2 + tamanio;
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 130, 0, 80, 80, TRANS_C, 40, 40, ptr_sol->x + 40 * escala, ptr_sol->y + 40 * escala, escala, escala, (float)ptr_sol->angulo * .01745, 0);
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 210, 0, 80, 80, NORM_C, 0, 0, ptr_sol->x, ptr_sol->y, escala, escala, 0, 0);
			}
			//Rotar el halo del sol
			if (frames % 2) {
				//Rota "clockwise"
				if (ptr_sol->angulo >= 0) {
					//Añade hasta los 360°
					if (ptr_sol->angulo < 360)
						ptr_sol->angulo++;
					else ptr_sol->angulo = 0;
				}
				//Rota "counterclockwise"
				else if (ptr_sol->angulo < 0) {
					//Añade hasta los -361°
					if (ptr_sol->angulo >= -360)
						ptr_sol->angulo--;
					else ptr_sol->angulo = -1;
				}
			}
		}
	}

	al_draw_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 0, 0, 130, 130, 10, 0, 0);
	al_draw_bitmap_region((ALLEGRO_BITMAP*)b.pala_interfaz, 0, 0, 110, 110, LIM_SEM * 110 + 150, 0, 0);

	//Pintar la cantidad de soles de amarillo si se están sumando
	color_aux = al_map_rgb(255, 255, 255);
	if (soles_guard_suma > 0) {
		color_aux = soles_guard / 4 < soles_guard_suma ? al_map_rgb(255, 220, 85) : al_map_rgb(255, 235, 145);
		soles_guard_suma--;
	}
	//Pintar la cantidad de soles de rojo si se están restando
	else if (soles_guard_suma < 0) {
		color_aux = soles_guard / 4 < -soles_guard_suma ? al_map_rgb(255, 105, 140) : al_map_rgb(255, 155, 180);
		soles_guard_suma += 3;
	}
	//Pintar la cantidad de soles de rojo si no hay soles
	if (!soles_guard)
		color_aux = al_map_rgb(255, 31, 95);
	dibujar_numero(soles_guard - soles_guard_suma, 70, 90, color_aux);

	//Dibujar semillero
	for (int i{}; i < LIM_SEM; i++) {
		if (planta_elegida != semillero[i].plant) {
			color_aux = soles_guard >= COST_PLANTA[semillero[i].plant] ? al_map_rgb(255, 255, 255) : al_map_rgb(255, 31, 95);
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, (semillero[i].plant - 1) * 110, 0, 110, 110, i * 110 + 150, 0, 0);
			if (semillero[i].recarga > 0) {
				al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_recarga, al_map_rgba(90, 80, 90, 211), 0, 0, 110, (((float)semillero[i].recarga / REC_PLANTA[semillero[i].plant]) * 60 + 8), i * 110 + 150, 0, 0);
			}
			dibujar_numero(COST_PLANTA[semillero[i].plant], i * 110 + 215, 74, color_aux);
		}
		else {
			al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, al_map_rgb(64, 64, 64), (semillero[i].plant - 1) * 110, 0, 110, 110, i * 110 + 150, 0, 0);
			dibujar_numero(COST_PLANTA[semillero[i].plant], i * 110 + 215, 74, al_map_rgb(64, 64, 64));
		}
	}

	//Dibujar planta elegida en el cursor

	if (planta_elegida) {
		if (planta_elegida >= 1 && planta_elegida <= 8) {
			al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.plantas_dia, al_map_rgba(64, 64, 64, 165), (planta_elegida - 1) * 90, 0, 90, 90, mouse_x - 45, mouse_y - 45, 0);
		}
		else if (planta_elegida == -1) {
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.pala_interfaz, 110, 0, 110, 110, LIM_SEM * 110 + 150, 0, 0);
			al_draw_bitmap((ALLEGRO_BITMAP*)b.pala_cursor, mouse_x - 55, mouse_y - 55, 0);
		}
	}

	//Dibujar en ventana
	al_flip_display();
}