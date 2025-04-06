#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_image.h>

#define CAS_X 9
#define CAS_Y 5
#define POS_X 1
#define LIM_SEM 7

static short planta_elegida{ 0 }, frames{};
static bool bucle{ 1 };
static short cont, soles_guard{}, plantas_en_semillero[LIM_SEM];

struct Sol{
	short estado_act;
	short cant;
	union {
		struct {
			short x;
			short y;
			float mov_x;
			float impulso;
			bool d;
		} anim;
		struct {
			short x;
			short y;
		} quiet;
		struct {
			short x, mov_x;
			short y, mov_y;
		} recol;
	} estado;
} sol_tablero[50];
static short cant_sol_tablero{};

struct Proyectil {
	short x, y, mov_x;
} *proyectil;

short cant_proy_tablero{};

struct {
	short plant, recarga, costo;
}static semillero[LIM_SEM];

struct {
	short pos, pv, tiemp, estado, tiemp_estado, animacion;
}static zombie[POS_X][CAS_Y];

struct Planta{
	short pos, pv, tiemp, estado, tiemp_evento, animacion;
}static planta[CAS_X][CAS_Y];

void registrar_teclas(ALLEGRO_EVENT);
void registrar_mouse(ALLEGRO_EVENT);
void plantar_planta(ALLEGRO_EVENT);
void seleccionar_planta(ALLEGRO_EVENT);

void funcion_sol(short, short, Sol&);
void funcion_planta();
void muerte_planta(short, short);

void generar_sol_recolect(short, short, short, Sol&);
void generar_proyectil(short, Proyectil&);

float animacion_sol(Sol&);
short animacion_planta(short pos_x, short pos_y);

int main() {
	srand(time(0));
	short mouse_x{}, mouse_y{};

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

	ALLEGRO_DISPLAY* display = al_create_display(1280, 720);
	ALLEGRO_EVENT_QUEUE* cola_eventos = al_create_event_queue();
	ALLEGRO_TIMER* tiempo = al_create_timer(1.0 / 60);
	ALLEGRO_EVENT eventos;
	ALLEGRO_BITMAP
		* plantas_dia_bitmap = al_load_bitmap("Sprites/Plants/Plants_Daytime.png"),
		* semillero_bitmap = al_load_bitmap("Sprites/Extra/Seedpackets_Bitmap.png"),
		* pala_cursor = al_load_bitmap("Sprites/Plants/Shovel.png"),
		* pala_interfaz = al_load_bitmap("Sprites/Extra/Shovel_Hud.png"),
		* fondo_soles = al_load_bitmap("Sprites/Extra/Sun_Counter.png"),
		* sol_recol = al_load_bitmap("Sprites/Extra/Sun.png"),
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
		if (eventos.type == ALLEGRO_EVENT_TIMER) {
			if (!(frames % 15)) {
				funcion_planta();
				for (int i{}; i < cant_sol_tablero; i++) {
					funcion_sol(mouse_x, mouse_y, sol_tablero[i]);
				}
			}

/*----------DIBUJADO------------------------------------------------------------------------------------------------------*/

			al_draw_bitmap(fondo_casa_dia, 0, 0, 0);
			al_draw_bitmap(fondo_soles, 10, -10, 0);
			al_draw_bitmap_region(pala_interfaz, 0, 0, 110, 110, LIM_SEM * 110 + 150, 0, 0);

			//Dibujar semillero

			for (int i{}; i < LIM_SEM; i++) {
				if (planta_elegida != semillero[i].plant) {
					al_draw_bitmap_region(semillero_bitmap, (semillero[i].plant - 1) * 110, 0, 110, 110, i * 110 + 150, 0, 0);
				}
				else {
					al_draw_tinted_bitmap_region(semillero_bitmap, al_map_rgb(64, 64, 64), (semillero[i].plant - 1) * 110, 0, 110, 110, i * 110 + 150, 0, 0);
				}
			}

			//Dibujar planta en el tablero

			for (int y{}; y < CAS_Y; y++) {
				for (int x{}; x < CAS_X; x++) {
					if (planta[x][y].pos) {
						if (planta[x][y].pos >= 0 && planta[x][y].pos <= 8) {
							al_draw_bitmap_region(plantas_dia_bitmap, (planta[x][y].pos - 1) * 90, animacion_planta(x, y), 90, 90, x * 100 + 195, y * 100 + 165, 0);
						}
					}
				}
			}

			//Dibujar proyectil en el tablero
			/* for (int i{}; i < cant_sol_tablero; i++) {
				if (sol_tablero[x].estado_act <= 1) {
					al_draw_bitmap(guisante, sol_tablero[i].estado.anim.x + sol_tablero[i].estado.anim.mov_x, animacion_sol(sol_tablero[i]), 0);
				}
			}*/

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
			//Dibujar soles en el tablero
			for (int i{}; i < cant_sol_tablero; i++) {
				if (sol_tablero[i].estado_act <= 1) {
					al_draw_bitmap(sol_recol, sol_tablero[i].estado.anim.x + sol_tablero[i].estado.anim.mov_x, animacion_sol(sol_tablero[i]), 0);
				}
				else if (sol_tablero[i].estado_act != 3) {
					al_draw_bitmap(sol_recol, sol_tablero[i].estado.quiet.x, sol_tablero[i].estado.quiet.y, 0);
				}
				else {
					if (sol_tablero[i].estado.recol.x > 55 && sol_tablero[i].estado.recol.y > 30) {
						if (sol_tablero[i].estado.recol.x < sol_tablero[i].estado.recol.mov_x / 5 ||
							sol_tablero[i].estado.recol.y < sol_tablero[i].estado.recol.mov_y / 5) {
							sol_tablero[i].estado.recol.x -= sol_tablero[i].estado.recol.mov_x / 90;
							sol_tablero[i].estado.recol.y -= sol_tablero[i].estado.recol.mov_y / 90;
						}
						else if (sol_tablero[i].estado.recol.x < sol_tablero[i].estado.recol.mov_x / 3 ||
							sol_tablero[i].estado.recol.y < sol_tablero[i].estado.recol.mov_y / 3) {
							sol_tablero[i].estado.recol.x -= sol_tablero[i].estado.recol.mov_x / 50;
							sol_tablero[i].estado.recol.y -= sol_tablero[i].estado.recol.mov_y / 50;
						}
						else {
							sol_tablero[i].estado.recol.x -= sol_tablero[i].estado.recol.mov_x / 30;
							sol_tablero[i].estado.recol.y -= sol_tablero[i].estado.recol.mov_y / 30;
						}
					}
					al_draw_bitmap(sol_recol, sol_tablero[i].estado.recol.x, sol_tablero[i].estado.recol.y, 0);
				}
			}

			al_flip_display();
			frames++;
		}

/*------DETECTAR-TECLAS---------------------------------------------------------------------------------------------------*/

		else {
			registrar_teclas(eventos);
			registrar_mouse(eventos);

			mouse_x = eventos.mouse.x;
			mouse_y = eventos.mouse.y;
		}
	}

	al_destroy_timer(tiempo);
	al_destroy_event_queue(cola_eventos);
	al_destroy_display(display);

	al_destroy_bitmap(fondo_casa_dia);
	al_destroy_bitmap(semillero_bitmap);
	al_destroy_bitmap(zombie_basico);
}

void registrar_mouse(ALLEGRO_EVENT mouse) {
	if (mouse.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
		switch (mouse.mouse.button) {
		case ALLEGRO_MOUSE_BUTTON_LEFT:
			if (mouse.mouse.y >= 160) {
				plantar_planta(mouse);
			}
			else {
				seleccionar_planta(mouse);
			}
			break;
		}
	}
}

void registrar_teclas(ALLEGRO_EVENT teclado) {
	if (teclado.type == ALLEGRO_EVENT_KEY_DOWN) {
		switch (teclado.keyboard.keycode) {
		case ALLEGRO_KEY_F:
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
		case ALLEGRO_KEY_1: planta_elegida = semillero[0].plant; break;
		case ALLEGRO_KEY_2: planta_elegida = semillero[1].plant; break;
		case ALLEGRO_KEY_3: planta_elegida = semillero[2].plant; break;
		case ALLEGRO_KEY_4: planta_elegida = semillero[3].plant; break;
		case ALLEGRO_KEY_5: planta_elegida = semillero[4].plant; break;
		case ALLEGRO_KEY_6: planta_elegida = semillero[5].plant; break;
		case ALLEGRO_KEY_7: planta_elegida = semillero[6].plant; break;
		case ALLEGRO_KEY_L: planta_elegida = -1; break;
		case ALLEGRO_KEY_ESCAPE: bucle = 0; break;
		}
	}
}

void seleccionar_planta(ALLEGRO_EVENT pos) {
	short pos_x{ -1 };
	for (int i{}; i <= LIM_SEM; i++) {
		if (pos.mouse.x >= 150 + i * 110 && pos.mouse.x < 260 + i * 110) {
			pos_x = i;
		}
	}
	if (pos_x != -1 && pos.mouse.y <= 110) {
		if (!planta_elegida) {
			if (pos_x != LIM_SEM) planta_elegida = semillero[pos_x].plant;
			else planta_elegida = -1;
		}
		else if (planta_elegida == semillero[pos_x].plant || (planta_elegida == -1 && pos_x == LIM_SEM)) {
			planta_elegida = 0;
		}
		else {
			if (pos_x != LIM_SEM) planta_elegida = semillero[pos_x].plant;
			else planta_elegida = -1;
		}
	}
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
			if (planta_elegida > 0 && !planta[pos_x][pos_y].pos) {
				planta[pos_x][pos_y].pos = planta_elegida;
				std::cout << "PLANTA " << planta_elegida << " PLANTADA EN \t[" << pos_x << "][" << pos_y << "]" << std::endl;
				
			}		
		}
		//DESPLANTAR UNA PLANTA
		else if (planta_elegida == -1 && planta[pos_x][pos_y].pos > 0) {
			planta[pos_x][pos_y].pos = 0;
			std::cout << "ASESINATO EN \t\t[" << pos_x << "][" << pos_y << "]" << std::endl;
		}
		planta_elegida = 0;
	}
}

void funcion_sol(short mouse_x, short mouse_y, Sol& sol) {
	short x, y;
	if (sol.estado_act != 3) {
		switch (sol.estado_act) {
		case 0:
			y = (short)animacion_sol(sol);
			x = sol.estado.anim.x + sol.estado.anim.mov_x;
			break;
		case 1:
			y = (short)animacion_sol(sol);
			x = sol.estado.anim.x + (short)sol.estado.anim.mov_x;
			sol.estado.quiet.x = x;
			sol.estado.quiet.y = y;
			sol.estado_act = 2;
			break;
		case 2:
			x = sol.estado.quiet.x;
			y = sol.estado.quiet.y;
			break;
		}
		if (mouse_x > x - 60 && mouse_x < x + 60 && mouse_y > y - 60 && mouse_y < y + 60) {
			soles_guard += sol.cant;
			std::cout << "SOL RECOLECTADO" << std::endl;
			sol.estado.recol.x = x;
			sol.estado.recol.y = y;
			sol.estado.recol.mov_x = x - 50;
			sol.estado.recol.mov_y = y - 30;
			sol.estado_act = 3;

		}
	}
}

void funcion_planta() {
	int cantZombie = 1;
	for (int x{}; x < CAS_X; x++) {
		for (int y{}; y < CAS_Y; y++) {
			if (planta[x][y].pos) {
				switch (planta[x][y].pos) {
				case 1://LANZAGUISANTES
					if (planta[x][y].estado == 0 && cantZombie == 1) {
						planta[x][y].tiemp++;
						planta[x][y].estado = 1;
						std::cout << "Zombie en fila [" << y << "]" << std::endl;
					}else if(planta[x][y].estado == 1){
						planta[x][y].tiemp++;
						if (planta[x][y].tiemp >= 2) {
							proyectil = new Proyectil;
							generar_proyectil(x * 100 + 195, *proyectil);
						}
					}
					break;
				case 2://GIRASOL
					if (planta[x][y].estado == 1) {
						//animación
						if (planta[x][y].animacion < 3) planta[x][y].animacion++;
						//Generar soles
						if (planta[x][y].tiemp > 5 + rand() % 3) {
							planta[x][y].animacion = rand() % 4;
							planta[x][y].tiemp = 0;
							planta[x][y].estado = 0;
							generar_sol_recolect(x * 100 + 195, y * 100 + 165, 25,sol_tablero[cant_sol_tablero]);
						}
					}
					else if (planta[x][y].estado == 0) {
						//animación planta
						if (planta[x][y].animacion < 3) planta[x][y].animacion++;
						else planta[x][y].animacion = 0;
						//cambio de estado: soles
						if (planta[x][y].tiemp >= 53 + rand() % 5) {
							planta[x][y].animacion = -1;
							planta[x][y].tiemp = 0;
							planta[x][y].estado = 1;
							std::cout << "SOL GENERANDO" << std::endl;
						}
					}
					break;

				case 4://NUEZ
					if ( planta[x][y].pv > 53) {
						planta[x][y].estado = 1;
					}
					else if(planta[x][y].pv > 31 && planta[x][y].pv <= 53) {
						planta[x][y].estado = 2;
					}
					else if (planta[x][y].pv > 10 && planta[x][y].pv <= 31) {
						planta[x][y].estado = 3;
					}
					else {
						muerte_planta(x, y);
					}
					break;

				case 5://PAPAPUM
					if (planta[x][y].estado == 0 && planta[x][y].tiemp >= 80) {
						planta[x][y].tiemp = 0;
						planta[x][y].estado = 1;
						std::cout << "PAPAPUM ARMADA" << std::endl;
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

short animacion_planta(short x, short y) {
	switch (planta[x][y].pos) {
	case 2://GIRASOL
		switch (planta[x][y].estado) {
		case 0:
			//NORMAL
			return planta[x][y].animacion * 90;
		case 1:
			//ANIMACIÓN SOLES
			return planta[x][y].animacion * 90 + 360;
		}
		break;
	case 4://NUEZ
		switch (planta[x][y].estado) {
		case 0:
			//NORMAL
			return 0;
		case 1:
			//POCO DAÑADA
			return 90;
		case 2:
			//DAÑADA
			return 180;
		case 3:
			//MUY DAÑADA
			return 270;
		}
		break;
	case 5://PAPAPUM
		switch (planta[x][y].estado) {
		case 0:
			//NORMAL
			return ((planta[x][y].tiemp >= 65 ? planta[x][y].tiemp : 0) % 2) * 90 + 180;
		case 1:
			//ACTIVA
			return (planta[x][y].tiemp % 2) * 90;
		}
		break;
	}
}

float animacion_sol(Sol& sol) {
	short direccion = sol.estado.anim.d * 2 - 1;
	if (direccion == 1) {
		if (sol.estado.anim.mov_x < sol.estado.anim.impulso * 2) {
			sol.estado.anim.mov_x += sol.estado.anim.impulso / (sol.estado.anim.impulso / 5 * 3);
		}
		if (sol.estado.anim.mov_x >= sol.estado.anim.impulso * 2) {
			sol.estado.anim.mov_x = sol.estado.anim.impulso * 2;
			sol.estado_act = 1;
		}
	}
	else if (direccion == -1) {
		if (sol.estado.anim.mov_x > sol.estado.anim.impulso * -2) {
			sol.estado.anim.mov_x -= sol.estado.anim.impulso / (sol.estado.anim.impulso / 5 * 3);
		}
		if (sol.estado.anim.mov_x <= sol.estado.anim.impulso * -2) {
			sol.estado.anim.mov_x = sol.estado.anim.impulso * -2;
			sol.estado_act = 1;
		}
	}
	return sol.estado.anim.y - sqrt(pow(sol.estado.anim.impulso, 2) - pow(sol.estado.anim.mov_x + sol.estado.anim.impulso * (-direccion), 2));
}

void generar_sol_recolect(short pos_x, short pos_y, short cant, Sol &sol) {
	sol.estado_act = 0;
	sol.cant = cant;
	sol.estado.anim.x = pos_x + 22;
	sol.estado.anim.y = pos_y + 45 - rand() % 4 * 10;
	sol.estado.anim.impulso = (rand() % 6 + 1) * 5;
	sol.estado.anim.d = rand() % 2;
	sol.estado.anim.mov_x = 0;
	cant_sol_tablero++;
}
void generar_proyectil(short pos_x, Proyectil &proyectil) {
	proyectil.x = pos_x + 5;
	cant_proy_tablero++;
	std::cout << "Proyectil generado en [" << pos_x << "]" << std::endl;

}

void muerte_planta(short x, short y) {
	planta[x][y].pos = 0;
	planta[x][y].pv = 0;
	planta[x][y].tiemp = 0;
	planta[x][y].estado = 0;
	planta[x][y].tiemp_evento = 0;
	planta[x][y].animacion = 0;
}
