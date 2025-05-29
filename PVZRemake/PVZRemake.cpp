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
#define LIM_SEM 6

#define LIM_F_PLANT 8
#define LIM_F_ZOMB 28
#define LIM_F_ZOMB_COM 12
#define LIM_F_EXPLO 4

#define FRAMES_INI 1800

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

static short planta_elegida{ 0 }, semillero_elegido{ 0 };
static unsigned long frames{ 0 };
static bool bucle{ true }, programa_corriendo{ true };
static short cont, soles_guard{ 50 }, soles_guard_suma{ 0 }, plantas_en_semillero[LIM_SEM];
static short cant_sol_tablero{ 0 }, cant_particulas{ 0 };
static short cant_zombi[CAS_Y]{ 0,0,0,0,0 }, cant_zombi_muerto[CAS_Y]{ 0,0,0,0,0 }, cant_proy[CAS_Y], anim_sobre_tablero{ 0 };
static short pantalla{ 0 }, pos_semillero{ 0 };
static short explosion_tablero[CAS_X + 1][CAS_Y];

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
	short pos, pv, estado;
	int tiemp, animacion;
	short anim_danio;
	//Especialidades de cada planta
	union {
		short mordidas;		//Nuez
		short coord_zomb;	//Chomper
		short repet;		//Repetidora
	}esp;
}		static  planta[CAS_X][CAS_Y];

struct Oleada {
	short posibilidadZomb;
	short zombieAtak;
	short difZomb, randomZombie, tiempNoZomb;
	short cantZomb;
	long long puntos;
	short dificultad, limiteDificultad, tiempoOleada, ritmoNivel, limiteTiempoOleada;
}static	oleada;

struct Imagenes {
	//Imágenes generales
	void* cursor_bitmap;
	void* fuente_bitmap;

	//Imágenes pantalla título
	void* titulo_bitmap;

	//Imágenes selector
	void* selector_hud;

	//Imágenes juego normal
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
	bool dir;
	Particula *ant_part, *sig_part;
}	static* particulas;

struct Cursor {
	short x, y, estado;
};

/*----------PROTOTIPOS----------------------------------------------------------------------------------------------------*/

//INPUT
void registrar_teclas(ALLEGRO_EVENT, Cursor);
void registrar_mouse(ALLEGRO_EVENT, Cursor&, bool);

//PLANTAS
void eliminar_planta(Planta&);
void funcion_planta(short, short);
void generar_planta(short, Planta&);
void plantar_planta(Cursor);
void seleccionar_planta(Cursor, short);
short animacion_planta(short, short, bool);

//SOL
bool funcion_sol(Cursor, Sol&, short);
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
void generar_particula(short, short, short, bool);
void rotacion_particula_x(Particula&);
float animacion_cabeza_part(Particula&);
void oleadas_zombie();

//DIBUJADO
void dibujar_numero(short, float, float, ALLEGRO_COLOR, Imagenes);
void dibujar_texto(char*, short, short, ALLEGRO_COLOR, Imagenes);
void dibujar_titulo(Imagenes, Cursor);
void dibujar_selector(Imagenes, Cursor);
void dibujar_tablero(Imagenes, Cursor);
void dibujar_cursor(Imagenes, Cursor);

//EXTRA
void funcion_semillero();

//INICIAR/ TERMINAR JUEGO
void inicializar_titulo(Imagenes&);
void finalizar_titulo(Imagenes&); 

void inicializar_selector(Imagenes&);
void finalizar_selector(Imagenes&);

void inicializar_juego(Imagenes&);
void finalizar_juego(Imagenes&);

/*----------FUNCIÓN-MAIN--------------------------------------------------------------------------------------------------*/

int main() {
	srand(time(0));
	Imagenes bitmap;
	Cursor mouse;
	bool cursor_org{ true };
	int pantalla_org;
	mouse.estado = 0;
	std::cout << "INICIO DE PROGRAMA" << std::endl;

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

	ALLEGRO_BITMAP* al_cursor_bitmap = al_load_bitmap("Sprites/Extra/Cursor_Bitmap.png");
	ALLEGRO_BITMAP* al_font_bitmap = al_load_bitmap("Sprites/Extra/Font_Bitmap.png");
	bitmap.cursor_bitmap = al_cursor_bitmap;
	bitmap.fuente_bitmap = al_font_bitmap;

	al_set_window_title(display, "Plantas Contra Zombies Remake");
	al_hide_mouse_cursor(display);

	//Registrar entradas. Teclado, mouse y tiempo
	al_register_event_source(cola_eventos, al_get_keyboard_event_source());
	al_register_event_source(cola_eventos, al_get_mouse_event_source());
	al_register_event_source(cola_eventos, al_get_timer_event_source(tiempo));

	//Iniciar el tiempo
	al_start_timer(tiempo);

	while (programa_corriendo) {
		switch (pantalla) {
		case 0: //Título
			inicializar_titulo(bitmap);

			//bucle título
			while (bucle) {
				pantalla_org = pantalla;
				al_wait_for_event(cola_eventos, &eventos);
				//Registrar posición del mouse
				if (eventos.type == ALLEGRO_EVENT_MOUSE_AXES) {
					mouse.x = eventos.mouse.x;
					mouse.y = eventos.mouse.y;
				}
				switch (eventos.type) {
					//Dibuja
				case ALLEGRO_EVENT_TIMER:
					dibujar_titulo(bitmap, mouse);
					break;

					//Registrar teclas
				case ALLEGRO_EVENT_KEY_DOWN:
					registrar_teclas(eventos, mouse);
					break;

					//Registrar botones mouse
				case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
					registrar_mouse(eventos, mouse, true);
					break;
				case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
					registrar_mouse(eventos, mouse, false);
					break;
				}//Switch eventos.type
				if (pantalla != pantalla_org) {
					bucle = false;
				}
			}//While Bucle

			finalizar_titulo(bitmap);
			break;

		case 1: //Selector
			inicializar_selector(bitmap);

			//Bucle selector
			while (bucle) {
				pantalla_org = pantalla;
				al_wait_for_event(cola_eventos, &eventos);
				//Registrar posición del mouse
				if (eventos.type == ALLEGRO_EVENT_MOUSE_AXES) {
					mouse.x = eventos.mouse.x;
					mouse.y = eventos.mouse.y;
				}
				switch (eventos.type) {
					//Dibuja
				case ALLEGRO_EVENT_TIMER:
					dibujar_selector(bitmap, mouse);
					break;

					//Registrar teclas
				case ALLEGRO_EVENT_KEY_DOWN:
					registrar_teclas(eventos, mouse);
					break;

					//Registrar botones mouse
				case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
					registrar_mouse(eventos, mouse, true);
					break;
				case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
					registrar_mouse(eventos, mouse, false);
					break;
				}//Switch eventos.type
				if (pantalla != pantalla_org) {
					bucle = false;
				}
			}//While Bucle

			finalizar_selector(bitmap);
			break;

		case 2: //Juego
			inicializar_juego(bitmap);

			//Bucle Juego
			while (bucle) {
				pantalla_org = pantalla;
				al_wait_for_event(cola_eventos, &eventos);
				//Registrar posición del mouse
				if (eventos.type == ALLEGRO_EVENT_MOUSE_AXES) {
					mouse.x = eventos.mouse.x;
					mouse.y = eventos.mouse.y;
				}
				switch (eventos.type) {
					//El juego avanza
				case ALLEGRO_EVENT_TIMER:
					//Revisar funciones
					oleadas_zombie();
					if (!(frames % 6)) {
						funcion_semillero();
						//Revisar soles
						if (cant_sol_tablero) {
							Sol* ptr_sol = NULL, * anterior_sol = NULL;
							ptr_sol = sol_tablero;
							for (int i{}; i < cant_sol_tablero; i++) {
								anterior_sol = ptr_sol;
								ptr_sol = ptr_sol->sig_sol;
								if (funcion_sol(mouse, *ptr_sol, i)) {
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
								cant_zombi_muerto[i] = 0;
								for (int j{}; j < cant_zombi[i]; j++) {
									anterior_zomb = ptr_zomb;
									ptr_zomb = ptr_zomb->sig_zomb;
									if (funcion_zombie(*ptr_zomb, i)) {
										ptr_zomb = anterior_zomb;
										j--;
									}
									else if (ptr_zomb->est_danio < 0) {
										cant_zombi_muerto[i]++;
									}
									ptr_zomb->prior = j;
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
					//Generar soles
					if (!(frames % 800)) {
						generar_sol_recolect(rand() % (RESOL_X - 500) + 300, 0, 25);
					}

					dibujar_tablero(bitmap, mouse);
					frames++;
					break;

					//Registrar teclas
				case ALLEGRO_EVENT_KEY_DOWN:
					registrar_teclas(eventos, mouse);
					break;

					//Registrar botones mouse
				case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
					registrar_mouse(eventos, mouse, true);
					break;
				case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
					registrar_mouse(eventos, mouse, false);
					break;

				}//Switch eventos.type
				if (pantalla != pantalla_org) {
					bucle = false;
				}
			}//While Bucle

			finalizar_juego(bitmap);
			break;

		}//Switch Pantalla

	}//El programa se termina

	//LIBERACIÓN DE ESPACIO
	al_destroy_timer(tiempo);
	al_destroy_event_queue(cola_eventos);
	al_destroy_display(display);
	al_destroy_bitmap(al_cursor_bitmap);
	al_destroy_bitmap(al_font_bitmap);
}

/*----------FUNCIONES-INPUT-----------------------------------------------------------------------------------------------*/

void registrar_mouse(ALLEGRO_EVENT evento, Cursor &mouse, bool presionado) {
	bool encontrado;
	switch (pantalla) {
	case 0://Pantalla de título
		switch (evento.mouse.button) {
		case ALLEGRO_MOUSE_BUTTON_LEFT:
			if (presionado) {
				mouse.estado = 2;
			}
			else {
				if (mouse.x >= 836 && mouse.x < 1115 && mouse.y >= 198 && mouse.y < 368) {
					pantalla = 1;
				}

				if (mouse.x >= 857 && mouse.x < 1099 && mouse.y >= 368 && mouse.y < 443) {
					//Muestra el marcador de puntuaciones
				}
				mouse.estado = 0;
			}
		}
		break;
	case 1://Selector
		switch (evento.mouse.button) {
		case ALLEGRO_MOUSE_BUTTON_LEFT:
			if (presionado) {
				//Buscar ratón en la selección de semillas
				if (mouse.y <= 110) {
					for (int i{}; i < pos_semillero; i++) {
						if (mouse.x >= i * 110 + 150 && mouse.x < i * 110 + 260 && pos_semillero) {
							for (int k{ i }; k <= pos_semillero; k++) {
								if (k + 1 < LIM_SEM) {
									semillero[k] = semillero[k + 1];
								}
								else {
									semillero[k].plant = -1;
								}
							}
							pos_semillero--;
						}
					}
				}
				//buscar ratón en el semillero
				if (mouse.y >= 190 && mouse.y < 300) {
					for (int i{}; i < CANT_PLANT; i++) {
						if (mouse.x >= i * 110 + 45 && mouse.x < i * 110 + 155) {
							encontrado = false;
							for (int j{}; j < pos_semillero; j++) {
								if (semillero[j].plant == i + 1) {
									encontrado = true;
									for (int k{ j }; k < pos_semillero; k++) {
										if (k + 1 < LIM_SEM) {
											semillero[k] = semillero[k + 1];
										}
										else {
											semillero[k].plant = -1;
										}
									}
									pos_semillero--;
								}
							}
							if (!encontrado && pos_semillero < LIM_SEM) {
								semillero[pos_semillero].plant = i + 1;
								pos_semillero++;
							}
						}
					}
				}
				mouse.estado = 2;
			}
			else {
				//Buscar ratón en el Botón de inicio ronda
				if (mouse.x >= 45 && mouse.x < 265 && mouse.y >= 445 && mouse.y < 575) {
					pantalla = 2;
				}//Buscar ratón en el Botón reinicio
				if (mouse.x >= 270 && mouse.x < 490 && mouse.y >= 445 && mouse.y < 575) {
					//WIP REINICIAR SEMILLERO
				}//buscar ratón en el Botón aleatorio
				if (mouse.x >= 495 && mouse.x < 620 && mouse.y >= 445 && mouse.y < 575) {
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
					pos_semillero = LIM_SEM;
				}
				mouse.estado = 0;
			}
		}
		break;
	case 2://En el juego
		if (presionado) {
			switch (evento.mouse.button) {
			case ALLEGRO_MOUSE_BUTTON_LEFT:
				if (mouse.y >= 160) {
					plantar_planta(mouse);
				}
				else {
					seleccionar_planta(mouse, 0);
				}
				break;
			}
		}
		break;
	}
}

void registrar_teclas(ALLEGRO_EVENT teclado, Cursor mouse) {
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
	case ALLEGRO_KEY_1: seleccionar_planta(mouse, 1); break;
	case ALLEGRO_KEY_2: seleccionar_planta(mouse, 2); break;
	case ALLEGRO_KEY_3: seleccionar_planta(mouse, 3); break;
	case ALLEGRO_KEY_4: seleccionar_planta(mouse, 4); break;
	case ALLEGRO_KEY_5: seleccionar_planta(mouse, 5); break;
	case ALLEGRO_KEY_6: seleccionar_planta(mouse, 6); break;
	case ALLEGRO_KEY_7: seleccionar_planta(mouse, 7); break;
	case ALLEGRO_KEY_L: seleccionar_planta(mouse, -1); break;
	case ALLEGRO_KEY_Z: generar_zombie(rand() % CAS_Y, rand()%3); break;
	case ALLEGRO_KEY_ESCAPE: bucle = false; programa_corriendo = false; break;
	}
}
 
/*----------FUNCIONES-PLANTAS---------------------------------------------------------------------------------------------*/

void eliminar_planta(Planta& planta) {
	planta.pos = 0;
	planta.pv = 0;
	planta.tiemp = 0;
	planta.estado = 0;
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
			if (planta[x][y].estado == 0 && cant_zombi[y] - cant_zombi_muerto[y] > 0 && planta[x][y].tiemp >= 12 + rand() % 4) {
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
					for (int sx{ -1 }; sx <= 1; sx++) {
						if (x + sx >= 0 && x + sx <= CAS_X) {
							for (int sy{ -1 }; sy <= 1; sy++) {
								if (y + sy >= 0 && y + sy < CAS_Y) {
									explosion_tablero[x + sx][y + sy] += 100;
								}
							}
						}
					}
					anim_sobre_tablero++;
				}
				break;
			case 1://Evita hacer daño doble
				if (planta[x][y].tiemp > 0) {
					for (int sx{ -1 }; sx <= 1; sx++) {
						if (x + sx >= 0 && x + sx <= CAS_X) {
							for (int sy{ -1 }; sy <= 1; sy++) {
								if (y + sy >= 0 && y + sy < CAS_Y) {
									explosion_tablero[x + sx][y + sy] -= 100;
								}
							}
						}
					}
					planta[x][y].estado = 2;
				}
				break;
			case 2://Termina de explotar
				if (planta[x][y].animacion > 11 ) {
					anim_sobre_tablero--;
					eliminar_planta(planta[x][y]);
				}
				break;
			}
		case 4://NUEZ
			planta[x][y].esp.mordidas = 0;
			break;
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
				if (planta[x][y].animacion >= 4 * LIM_F_PLANT - 1) {
					planta[x][y].estado = 3;
					planta[x][y].tiemp = 0;
					planta[x][y].animacion = 0;
					anim_sobre_tablero++;
				}
				break;
			case 3://Ya no hace daño acumulado
				planta[x][y].estado = 4;
				planta[x][y].tiemp = 0;
				break;
			case 4://Explota animación
				if (planta[x][y].tiemp > 25) {
					anim_sobre_tablero--;
					eliminar_planta(planta[x][y]);
				}
				break;
			}
			break;
		case 6://Hielaguisantes
			if (planta[x][y].estado == 0 && cant_zombi[y] - cant_zombi_muerto[y] > 0 && planta[x][y].tiemp >= 12 + rand() % 5) {
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
			switch (planta[x][y].estado) {
			case 1://No comió el zombie
				if (planta[x][y].animacion >= 4 * LIM_F_PLANT - 1) {
					planta[x][y].estado = 0;
					planta[x][y].tiemp = 0;
					planta[x][y].animacion = 0;
					anim_sobre_tablero--;
				}
				break;
			case 2://Muerde
				if (planta[x][y].animacion >= 15 * LIM_F_PLANT - 1) {
					planta[x][y].estado = 3;
					planta[x][y].tiemp = 0;
					planta[x][y].animacion = 0;
					anim_sobre_tablero--;
				}
				break;
			case 3://Come
				if (planta[x][y].tiemp >= 250) {
					planta[x][y].estado = 0;
					planta[x][y].animacion = 0;
				}
				break;
			}
			break;
		case 8://Repetidora
			if (planta[x][y].estado == 0 && cant_zombi[y] - cant_zombi_muerto[y] > 0 && planta[x][y].tiemp >= 12 + rand() % 5) {
				planta[x][y].tiemp = -1;
				planta[x][y].estado = 1;
				planta[x][y].esp.repet = 0;
				planta[x][y].animacion = 2 * LIM_F_PLANT - LIM_F_PLANT / 4 * 3;
			}
			if (planta[x][y].estado == 1) {
				if (planta[x][y].tiemp >= 3) {
					generar_proyectil(proyectil[y], y, planta[x][y].pos, x);
					planta[x][y].tiemp = 0;
					planta[x][y].animacion = 2 * LIM_F_PLANT - LIM_F_PLANT / 4 * 3;
					planta[x][y].esp.repet++;
				}
				if (planta[x][y].esp.repet >= 2) {
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
	planta.animacion = 0;
	planta.anim_danio = 0;
	switch (id_planta) {
	case 4://Nuez
		planta.esp.mordidas = 0;
		break;
	case 7://Carroñívora
		planta.esp.coord_zomb = 0;
		break;
	case 8://Repetidora
		planta.esp.repet = 0;
		break;
	}
	std::cout << "PV[" << id_planta << "] = " << planta.pv << std::endl;
}

void plantar_planta(Cursor mouse) {
	short pos_x{ -1 }, pos_y{ -1 };
	for (int i{}; i < CAS_X; i++) {
		if (mouse.x >= 190 + i * 100 && mouse.x < 290 + i * 100) {
			pos_x = i;
		}
	}
	for (int i{}; i < CAS_Y; i++) {
		if (mouse.y >= 160 + i * 100 && mouse.y < 260 + i * 100) {
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

void seleccionar_planta(Cursor mouse, short pos_semillero) {
	short pos_x{ -1 };
	for (int i{}; i <= LIM_SEM; i++) {
		if (mouse.x >= 150 + i * 110 && mouse.x < 260 + i * 110) {
			pos_x = i;
		}
	}
	if (pos_semillero) {
		if (pos_semillero > 0)
			pos_x = pos_semillero - 1;
		else
			pos_x = LIM_SEM;
	}
	mouse.y = 0;
	if (pos_x != -1 && mouse.y <= 110) {
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
			if (planta[x][y].animacion < 15 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			else if (planta[x][y].animacion >= 15 * LIM_F_PLANT - 1) planta[x][y].animacion = 0;
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0: case 8://CENTRO
				return 0;

			//SE INCLINA HACIA LA IZQUIERDA
			case 1: case 7:
				return 90;
			case 2: case 6:
				return 180;
			case 3: case 5:
				return 270;
			case 4:
				return 360;

			//SE INCLINA HACIA LA DERECHA
			case 9: case 15:
				return 450;
			case 10: case 14:
				return 540;
			case 11: case 13:
				return 630;
			case 12:
				return 720;
			}
		case 1://ANIMACIÓN SOLES
			if (planta[x][y].animacion < 4 && !(frames % 7)) planta[x][y].animacion++;
			else if (planta[x][y].animacion == 4 && planta[x][y].tiemp >= 11) planta[x][y].animacion++;
			return planta[x][y].animacion * 90 + 810;
		}
		break;

	case 3://PETACEREZA
		switch (planta[x][y].estado) {
		case 0:
			if (planta[x][y].animacion < 6 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			return int(planta[x][y].animacion / LIM_F_PLANT) * 90;
		default:
			if (!(frames % (LIM_F_EXPLO))) planta[x][y].animacion++;
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
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0:
				return 360 + ((planta[x][y].tiemp / 2) % 2 * 270);
			case 1:
				return 450 + ((planta[x][y].tiemp / 2) % 2 * 270);
			case 3: case 4:
				return 540 + ((planta[x][y].tiemp / 2) % 2 * 270);
			default:
				return ((planta[x][y].tiemp / 2) % 2) * 90 + (planta[x][y].tiemp % 22 ? 0 : 1) * 900;
			}
		case 2:
			if (planta[x][y].animacion < 4 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			return int(planta[x][y].animacion / LIM_F_PLANT) * 90 + 1080;
		case 3: case 4:
			if (planta[x][y].animacion < 9 * LIM_F_EXPLO - 1) planta[x][y].animacion++;
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
		case 1: case 2://ATAQUE
			if (corte_vert) {
				if (planta[x][y].animacion < 15 * LIM_F_PLANT - 1) planta[x][y].animacion++;
				switch (planta[x][y].animacion / LIM_F_PLANT) {
				case 5: case 8: case 11:
					return 900;

				case 6: case 9: case 12:
					if (!(rand() % 32)) {
						generar_particula(x * 100 + 168, y * 100 + 130, rand() % 3 + 5, rand() % 2);
					}
					return 1260;

				case 7: case 10: case 13:
					return 1080;

				case 14:
					return 1440;
				default:
					return int(planta[x][y].animacion / LIM_F_PLANT) * 180;
				}
			}
			else {
				return -90;
			}
		case 3://COME
			if (planta[x][y].animacion < 4 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			else if (planta[x][y].animacion >= 4 * LIM_F_PLANT - 1) planta[x][y].animacion = 0;
			return int(planta[x][y].animacion / LIM_F_PLANT) * 90 + 450;
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

bool funcion_sol(Cursor mouse, Sol& sol, short pos) {
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
		if (mouse.x > x - 160 && mouse.x < x + 160 && mouse.y > y - 160 && mouse.y < y + 160) {
			std::cout << "SOL RECOLECTADO" << std::endl;
			soles_guard += sol.cant;
			soles_guard_suma += sol.cant;
			oleada.puntos += 10;
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
	if (proy.tipo >= 0) {
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
				proy.tipo = -1;
				break;
			case 1://Guisante congelado WIP
				zomb_atacado->pv -= 1;
				zomb_atacado->estado = 1;
				zomb_atacado->tiemp = 60;
				proy.tipo = -2;
				break;
			}
			if (zomb_atacado->anim_danio < 18) {
				zomb_atacado->anim_danio = 24;
			}
			proy.tiempo_ev = 0;
		}
	}
	else {
		if (proy.tiempo_ev >= 4) {
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
	if (proy.tipo < 0) {
		proy.x += 1;
		return 0;
	}
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

		//generar partículas
		switch (zomb.id) {
		case 1://Caracono
			if (zomb.pv <= 17 && zomb.est_danio == 2) {
				zomb.est_danio = 3;
			}
			else if (zomb.pv <= 10 && zomb.est_danio == 3) {
				zomb.est_danio = 0;
				generar_particula(zomb.x, fila * 100 + 120, 1, 1);
			}
			break;
		case 2://Cubeta
			if (zomb.pv <= 23 && zomb.est_danio == 2) {
				zomb.est_danio = 3;
			}
			else if (zomb.pv <= 10 && zomb.est_danio == 3) {
				zomb.est_danio = 0;
				generar_particula(zomb.x, fila * 100 + 120, 2, 1);
			}
			break;
		}
		if (zomb.pv < 5 && zomb.est_danio == 0) {
			zomb.est_danio = 1;
			generar_particula(zomb.x, fila * 100 + 120, 5, 1);
		}

		//Encontrar Casilla
		for (int i{}; i <= CAS_X + 1; i++) {
			if (zomb.x >= 150 + i * 100 && zomb.x < 250 + i * 100) {
				casilla = i;
			}
		}

		//Rebajar tiempo estado
		if (zomb.estado == 1 && zomb.tiemp > 0) {
			zomb.tiemp--;
		}
		else {
			zomb.estado = 0;
		}

		//Busca en las casillas del juego
		if (casilla >= 0) {

			//Busca por explosiones en el tablero del juego
			if (explosion_tablero[casilla][fila] > 0 && casilla <= CAS_X) {
				zomb.pv -= explosion_tablero[casilla][fila];
				if (zomb.pv < 0) {
					zomb.est_danio = -4;
					zomb.animacion = 0;
					zomb.tiemp = 0;
				}
			}

			//Revisa las plantas y eventos en las casillas
			if (casilla < CAS_X) {
				//busca si se encuentra con una planta
				if (planta[casilla][fila].pos) {
					if (!(zomb.estado == 1 && frames % 20)) {
						switch (planta[casilla][fila].pos) {
						case 4://Nuez
							if (planta[casilla][fila].esp.mordidas < 3) {//es una nuez
								planta[casilla][fila].pv--;
								planta[casilla][fila].esp.mordidas++;
							}
							if (!planta[casilla][fila].anim_danio) { //animación de daño de la planta
								planta[casilla][fila].anim_danio = 24;
							}
							zomb.comiendo = true;
							break;
						case 3://Petacereza
							break;
						case 5://Papapum
							if (planta[casilla][fila].estado) {
								break;
							}
						default://Cualquier otra planta se la come
							planta[casilla][fila].pv--;
							if (!planta[casilla][fila].anim_danio) { //animación de daño de la planta
								planta[casilla][fila].anim_danio = 24;
							}
							zomb.comiendo = true;
							break;
						}
					}
				}
				//Si no hay planta, continúa caminando
				else {
					zomb.comiendo = false;
				}
			}

			//Revisar casilla y media más adelante
			if ((int(zomb.x) - 150) % 100 <= 50) {
				sx = -2;
			}
			else {
				sx = -1;
			}

			//Si se encuentra con una papapum ya armada
			if (planta[casilla][fila].pos == 5 && planta[casilla][fila].estado == 1 && planta[casilla][fila].animacion >= 7 * LIM_F_PLANT - 1) {
				planta[casilla][fila].estado = 2;
				planta[casilla][fila].animacion = 0;
				planta[casilla][fila].tiemp = 0;
			}

			//Busca una papapum
			for (int x{ sx + 1 }; x <= 0; x++) {
				if (planta[casilla + x][fila].pos == 5&& planta[casilla + x][fila].estado == 3) {
					zomb.pv -= 200;
					if (zomb.pv <= 0) {
						if (zomb.est_danio >= 2) {
							switch (zomb.id) { //Suelta el gorrito
							case 1: generar_particula(zomb.x, fila * 100 + 120, zomb.est_danio == 2 ? 3 : 1, 1); break;
							case 2: generar_particula(zomb.x, fila * 100 + 120, zomb.est_danio == 2 ? 4 : 2, 1); break;
							}
						}
						//Genera una partícula brazo, pierna o corbata
						generar_particula(zomb.x, fila * 100 + 120, rand() % 3 + 5, 1);
						goto eliminar_zombie;
					}
				}
			}

			//Busca una carroñivora
			for (int x{ sx }; x <= 0; x++) {
				//Encuentra planta carroñivora
				if (planta[casilla + x][fila].pos == 7 && casilla + x >= 0) {
					switch (planta[casilla + x][fila].estado) {
					case 0://Carroñivora muerde
						planta[casilla + x][fila].estado = 1;
						planta[casilla + x][fila].tiemp = 0;
						planta[casilla + x][fila].animacion = 0;
						anim_sobre_tablero++;
						break;
					case 1://Carroñivora hace daño
						if (int(planta[casilla + x][fila].animacion / LIM_F_PLANT) == 2) {
							if (zomb.est_danio >= 2) {
								switch (zomb.id) {
								case 1: generar_particula(zomb.x, fila * 100 + 120, zomb.est_danio == 2 ? 3 : 1, 1); break;
								case 2: generar_particula(zomb.x, fila * 100 + 120, zomb.est_danio == 2 ? 4 : 2, 1); break;
								}
							}
							generar_particula(zomb.x, fila * 100 + 120, 0, 1);
							planta[casilla + x][fila].esp.coord_zomb = zomb.x;
							planta[casilla + x][fila].estado = 2;
							goto eliminar_zombie;
						}
					default:
						break;
					}
				}
			}
		}
	}
	//Se muere
	else {
		zomb.tiemp++;
		if (zomb.est_danio >= 0) {
			generar_particula(zomb.x, fila * 100 + 120, 0, 1);
			zomb.tiemp = 0;
			zomb.est_danio = -1;
		}
		//Se mueve antes de morir
		if (zomb.tiemp >= 12 && zomb.est_danio == -1) {
			zomb.comiendo = false;
			zomb.tiemp = 0;
			zomb.animacion = 0;
			zomb.est_danio = -2;
		}
		//Fallece
		if (zomb.animacion >= 6 * (LIM_F_ZOMB * .3) - 1 && zomb.est_danio == -2) {
			zomb.est_danio = -3;
			zomb.tiemp = 0;
		}
		//Se termina de quemar
		if (zomb.animacion >= 14 * LIM_F_EXPLO - 1 && zomb.est_danio == -4) {
			zomb.est_danio = -5;
			zomb.tiemp = 0;
		}
		if (zomb.tiemp > 38) {
		eliminar_zombie:
			switch (zomb.id) {
			case 0:	oleada.puntos += 100;	break;
			case 1:	oleada.puntos += 200;	break;
			case 2:	oleada.puntos += 500;	break;
			}
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
			return true;
		}
	}
	return false;
}

void mover_zombie(Zombie& zomb, short fila) {
	if (!zomb.comiendo) {
		switch (zomb.est_danio) {
		default:
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
			break;
		case -2:
			zomb.x -= 0.06;
			break;
		case -3: case -4: case -5:
			break;
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
	oleada.cantZomb = 0;
	for (int i{}; i < CAS_Y; i++) {
		oleada.cantZomb += cant_zombi[i];
	}
	if (oleada.cantZomb == 0 && frames > FRAMES_INI) {
		oleada.tiempNoZomb++;
		std::cout << "TNZ: " << oleada.tiempNoZomb << std::endl;
		if (oleada.tiempNoZomb >= 240) {
			oleada.puntos += 1000;
			oleada.tiempNoZomb = 0;
			goto generar_nueva_oleada;
		}
	}
	else if (oleada.cantZomb > 0 && oleada.tiempNoZomb > 0) {
		oleada.tiempNoZomb = 0;
	}
	if ((frames - FRAMES_INI) % oleada.tiempoOleada == 0 && frames > FRAMES_INI-1) {
		generar_nueva_oleada:
		oleada.zombieAtak += oleada.dificultad;//Bajar el tiempo para la siguiente oleada
		if (oleada.tiempoOleada > oleada.limiteTiempoOleada && frames > FRAMES_INI) {
			oleada.tiempoOleada -= (oleada.tiempoOleada * .06) + (oleada.puntos + frames) * .00001 - 1;//MODIFICADO 0.8 org
		}//Si es menor a limiteTiempoOleada, se hace limiteTiempoOleada
		if (oleada.tiempoOleada < oleada.limiteTiempoOleada) {
			oleada.tiempoOleada = oleada.limiteTiempoOleada;
		}
		std::cout << "Inicio nueva Oleada " << oleada.cantZomb << std::endl;
	}
	if (frames % 60 == 0) {
		bool zomb_aparecido[CAS_Y], lleno{ false };
		for (int i{}; i < CAS_Y; i++) {
			zomb_aparecido[i] = false;
		}
		while (oleada.zombieAtak > 0 && !lleno) {
			short fila_aparicion;
			oleada.randomZombie = rand() % oleada.zombieAtak + 1;
			for (int i{}, cant_aparicion{}; i < CAS_Y; i++) {
				if (zomb_aparecido[i]) {
					cant_aparicion++;
				}
				if (cant_aparicion == CAS_Y) {
					lleno = true;
				}
			}
			if (lleno) {
				break;
			}
			do {
				fila_aparicion = rand() % CAS_Y;
			} while (zomb_aparecido[fila_aparicion]);
			if (oleada.randomZombie <= 1) {
				oleada.difZomb = 0;
				oleada.zombieAtak -= 1;
			}
			else if (oleada.zombieAtak > 1 && oleada.zombieAtak <= 3) {
				oleada.difZomb = 1;
				oleada.zombieAtak -= 2;
			}
			else if (oleada.zombieAtak > 3){
				oleada.difZomb = 2;
				oleada.zombieAtak -= 3;
			}
			zomb_aparecido[fila_aparicion] = true;
			generar_zombie(fila_aparicion, oleada.difZomb);
		}
	}
	if (frames > FRAMES_INI && (frames - FRAMES_INI) % oleada.ritmoNivel == 0 && oleada.dificultad < oleada.limiteDificultad) {
		oleada.dificultad++;
		if (oleada.ritmoNivel > FRAMES_INI/2) {
			oleada.ritmoNivel *= 0.85;
		}
		if (oleada.ritmoNivel < FRAMES_INI/2) {
			oleada.ritmoNivel = FRAMES_INI/2;
		}
	}
}

short animacion_zombie(Zombie& zomb) {
	switch (zomb.id) {
	case 0: case 1: case 2:
		if (zomb.comiendo) {
			if (zomb.animacion < 8 * LIM_F_ZOMB_COM - 1) zomb.animacion++;
			else if (zomb.animacion >= 8 * LIM_F_ZOMB_COM - 1) zomb.animacion = 0;
			switch (zomb.animacion / LIM_F_ZOMB_COM) {
			default:
				return 0;
			case 1: case 7:
				return 108;
			case 2: case 6:
				return 216;
			case 3: case 5:
				return 324;
			case 4:
				return 432;
			}
		}
		switch (zomb.est_danio) {
		default://Caminata default
			if (zomb.estado == 1 && frames % 2) {
				zomb.animacion--;
			}
			if (zomb.animacion < 12 * LIM_F_ZOMB - 1) zomb.animacion++;
			else if (zomb.animacion >= 12 * LIM_F_ZOMB - 1) zomb.animacion = 0;
			return int(zomb.animacion / LIM_F_ZOMB) * 108;

		case -2: case -3://Animación muerte "natural"
			if (zomb.estado == 1 && frames % 2) {
				zomb.animacion--;
			}
			if (zomb.animacion < 6 * (LIM_F_ZOMB * .3) -1) zomb.animacion++;
			return int(zomb.animacion / (LIM_F_ZOMB * .3)) * 108;

		case -4: case -5://Animación muerte quemado
			if (zomb.animacion < 20 * LIM_F_EXPLO - 1) zomb.animacion++;
			//Probabilidad de que parpadee de nuevo
			if (zomb.animacion == 10 * LIM_F_EXPLO && rand() % 2){
				zomb.animacion -= 4 * LIM_F_EXPLO;
			}
			//Se queda atontado
			if (zomb.animacion < 6 * LIM_F_EXPLO) {
				return 0;
			}
			switch (int(zomb.animacion / LIM_F_EXPLO) - 6) {
			//Parpadea
			case 0: case 1:
				zomb.tiemp = 0;
				return 0;
				break;
			case 2: case 3:
				return 108;
				break;
			//Se desmorona
			default:
				return (int(zomb.animacion / LIM_F_EXPLO) - 8) * 108;
				break;
			}
			break;
		}
		break;
	}
	return 0;
}

/*----------FUNCIONES-PARTÍCULAS------------------------------------------------------------------------------------------*/

bool funcion_particula(Particula& part) {
	bool eliminar{ false };
	if (part.tiemp > 1800) {
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
		return true;
	}
	return false;
}

void generar_particula(short x, short y, short id, bool dir) {
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
	case 6://corbata
		nueva_part->estado = 0;
		nueva_part->x = x + 54;
		nueva_part->mov_x = 0;
		nueva_part->impulso = rand() % 22 + 20;
		nueva_part->angulo = 0;
		nueva_part->tiemp = 0;
		nueva_part->y = y + 32;
		break;
	case 7://pierna
		nueva_part->estado = 0;
		nueva_part->x = x + 56;
		nueva_part->mov_x = 0;
		nueva_part->impulso = rand() % 16 + 20;
		nueva_part->angulo = 0;
		nueva_part->tiemp = 0;
		nueva_part->y = y + 34;
		break;
	}
	nueva_part->dir = dir;
	cant_particulas++;
}

void rotacion_particula_x(Particula& part) {
	short dir = part.dir ? 1 : -1;
	switch (part.id) {
	case 0:
		part.angulo += part.impulso * dir / 4;
		break;
	case 1: case 3:
		part.angulo += part.impulso * dir / 6;
		break;
	case 2: case 4:
		part.angulo += part.impulso * dir / 8;
		part.impulso--;
		break;
	case 5: case 6: case 7:
		part.angulo += part.impulso * dir / 12;
		part.impulso--;
		break;
	}
	part.x += dir;
	part.impulso--;
	if (part.impulso <= 0) {
		part.estado = 3;
		part.tiemp = 0;
	}
}

float animacion_cabeza_part(Particula& part) {
	float mult{ 1.5 }, caida{ 1 };
	short dir = part.dir ? 1 : -1;
	if (!part.estado) {
		//La partícula es una cabeza
		if (part.id != 5 && part.id != 6) {
			part.angulo += (part.impulso * dir - 18) / 3;
		}
		//La partícula es un brazo
		else {
			part.angulo += (part.impulso * dir - 8) / 5;
		}
	}
	//Derecha
	if (part.dir) {
		//Se traslada hacia la derecha
		if (part.mov_x < part.impulso * 2) {
			part.mov_x += part.impulso / (part.impulso / 6 * 3);
		}
		//Se detiene al llegar al final de la función matemática
		if (part.mov_x >= part.impulso * 2) {
			part.mov_x = part.impulso * 2;
			part.estado = 1;
		}
	}
	else {
		//Se traslada hacia la izquierda
		if (part.mov_x > part.impulso * -2) {
			part.mov_x -= part.impulso / (part.impulso / 6 * 3);
		}
		//Se detiene al llegar al final de la función matemática
		if (part.mov_x <= part.impulso * -2) {
			part.mov_x = part.impulso * -2;
			part.estado = 1;
		}
	}
	//Caso especial para la partícula brazo y pierna
	if (part.id == 5 || part.id == 6) {
		mult = .75;
		caida = 1;
	}
	//Retornar extra en y
	return part.y - sqrt(pow(float(part.impulso), 2) - pow(part.mov_x + part.impulso * (-dir), 2)) * mult + part.mov_x * caida * dir;
}

/*----------FUNCIONES-EXTRA-----------------------------------------------------------------------------------------------*/

void funcion_semillero() {
	for (int i{}; i < LIM_SEM; i++) {
		if (semillero[i].recarga > 0) {
			semillero[i].recarga--;
		}
	}
}

/*----------OPERACIONES-JUEGO---------------------------------------------------------------------------------------------*/

void inicializar_titulo(Imagenes& b) {
	ALLEGRO_BITMAP
		* titulo_bitmap = al_load_bitmap("Sprites/Start_Menu_bitmap.png");

	b.titulo_bitmap = titulo_bitmap;
	bucle = true;
}

void finalizar_titulo(Imagenes& b) {
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.titulo_bitmap);
}

void inicializar_selector(Imagenes& b) {
	for (int i{}; i < LIM_SEM; i++) {
		semillero[i].plant = -1;
	}

	ALLEGRO_BITMAP
		* semillero_bitmap = al_load_bitmap("Sprites/Extra/Seedpackets_Bitmap.png"),
		* selector_hud = al_load_bitmap("Sprites/Extra/Seed_selection_hud.png"),
		* fondo_casa_dia = al_load_bitmap("Sprites/Daylight_Playground.png");

	b.semillero_bitmap = semillero_bitmap;
	b.selector_hud = selector_hud;
	b.fondo_casa_dia = fondo_casa_dia;
	bucle = true;
}

void finalizar_selector(Imagenes& b) {
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.selector_hud);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.semillero_bitmap);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.fondo_casa_dia);
}

void inicializar_juego(Imagenes& b) {
	//Inicializar soles
	sol_tablero = new Sol;
	sol_tablero->ant_sol = NULL;
	sol_tablero->sig_sol = NULL;
	sol_tablero->estado_act = 4;

	//Inicializar partículas
	particulas = new Particula;
	particulas->ant_part = NULL;
	particulas->sig_part = NULL;
	particulas->id = -1;

	//Inicializar Proyectiles
	for (int i{}; i < CAS_Y; i++) {
		proyectil[i] = new Proyectil;
		proyectil[i]->ant_proy = NULL;
		proyectil[i]->sig_proy = NULL;
		cant_proy[i] = 0;
	}
	//Incializar Zombies
	for (int i{}; i < CAS_Y; i++) {
		zombie[i] = new Zombie;
		zombie[i]->id = -1;
		zombie[i]->ant_zomb = NULL;
		zombie[i]->sig_zomb = NULL;
		cant_zombi[i] = 0;
	}
	// inicializar Oleadas
	oleada.zombieAtak = 0;
	oleada.difZomb = 0;
	oleada.randomZombie = 0;
	oleada.tiempNoZomb = 0;
	oleada.dificultad = 1;
	oleada.limiteDificultad = 50;
	oleada.tiempoOleada = FRAMES_INI;
	oleada.ritmoNivel = FRAMES_INI * 3;
	oleada.limiteTiempoOleada = 360;
	//iniciar matriz plantas
	for (int y{}; y < CAS_Y; y++) {
		for (int x{}; x < CAS_X; x++) {
			planta[x][y].pos = 0;
			planta[x][y].pv = 0;
			planta[x][y].tiemp = 0;
			planta[x][y].estado = 0;
			planta[x][y].animacion = 0;
		}
	}

	for (int y{}; y < CAS_Y; y++) {
		for (int x{}; x <= CAS_X; x++) {
			explosion_tablero[x][y] = 0;
		}
	}

	frames = 1;

	ALLEGRO_BITMAP
		* plantas_dia_bitmap = al_load_bitmap("Sprites/Plants/Plants_Daytime.png"),
		* chomper_anim_bitmap = al_load_bitmap("Sprites/Plants/Chomper_Bite.png"),
		* semillero_bitmap = al_load_bitmap("Sprites/Extra/Seedpackets_Bitmap.png"),
		* semillero_recarga = al_load_bitmap("Sprites/Extra/Delay_Seedpacket.png"),
		* pala_cursor = al_load_bitmap("Sprites/Plants/Shovel.png"),
		* pala_interfaz = al_load_bitmap("Sprites/Extra/Shovel_Hud.png"),
		* sol_bitmap = al_load_bitmap("Sprites/Extra/Sun_Bitmap.png"),
		* explosion = al_load_bitmap("Sprites/Extra/Explosion_Bitmap.png"),
		* guisante = al_load_bitmap("Sprites/Bullets/Bullet_Pea.png"),
		* zombie_bitmap = al_load_bitmap("Sprites/Zombies/Zombie_Basic.png"),
		* fondo_casa_dia = al_load_bitmap("Sprites/Daylight_Playground.png");

	b.plantas_dia = plantas_dia_bitmap;
	b.chomper_anim = chomper_anim_bitmap;
	b.semillero_bitmap = semillero_bitmap;
	b.semillero_recarga = semillero_recarga;
	b.pala_cursor = pala_cursor;
	b.pala_interfaz = pala_interfaz;
	b.sol_bitmap = sol_bitmap;
	b.explosion = explosion;
	b.guisante = guisante;
	b.zombie_bitmap = zombie_bitmap;
	b.fondo_casa_dia = fondo_casa_dia;
	bucle = true;
}

void finalizar_juego(Imagenes& b) {
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
		}
		else {
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

	al_destroy_bitmap((ALLEGRO_BITMAP*)b.fondo_casa_dia);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.chomper_anim);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.plantas_dia);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.semillero_bitmap);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.pala_interfaz);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.pala_cursor);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.sol_bitmap);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.semillero_recarga);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.explosion);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.guisante);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.zombie_bitmap);

	//Inicializar las imágenes en null
	b.plantas_dia = b.chomper_anim = b.semillero_bitmap = 
	b.semillero_recarga = b.pala_cursor = b.pala_interfaz =
	b.sol_bitmap = b.explosion = b.guisante = 
	b.zombie_bitmap = b.fondo_casa_dia = NULL;
}

/*----------------------DIBUJADO------------------------------------------------------------------------------------------*/

void dibujar_numero(short num, float x, float y, ALLEGRO_COLOR color, Imagenes b) {
	short tam{}, copi_num{ num };
	float pos_x;
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
		al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, (copi_num % 10) * 30, 0, 30, 30, x + pos_x, y, 0);
	}
}

void dibujar_texto(char* text, short x, short y, ALLEGRO_COLOR color, Imagenes b) {
	short tam, fila{ 0 }, espacio{ 0 }, extra{ 0 }, renglon{ 0 };
	char caracter;

	tam = strlen((char*)text);
	for (int i{ 0 }; i < tam; i++, espacio += 22) {
		caracter = toupper(text[i]);
		fila = -1;
		switch (caracter) {
		case 39:
			if (i + 1 < tam) {
				switch (toupper(text[i + 1])) {
				case 'N':
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, 180, 90, 30, 30, x + espacio, y - 14, 0);
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, 90, 60, 30, 30, x + espacio, y, 0);
					break;
				case 'A':
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, 210, 90, 30, 30, x + espacio + 2, y - 14, 0);
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, 0, 30, 30, 30, x + espacio, y, 0);
					break;
				case 'E':
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, 210, 90, 30, 30, x + espacio + 2, y - 14, 0);
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, 120, 30, 30, 30, x + espacio, y, 0);
					break;
				case 'I':
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, 210, 90, 30, 30, x + espacio + 2, y - 14, 0);
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, 240, 30, 30, 30, x + espacio, y, 0);
					break;
				case 'O':
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, 210, 90, 30, 30, x + espacio + 2, y - 14, 0);
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, 120, 60, 30, 30, x + espacio, y, 0);
					break;
				case 'U':
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, 210, 90, 30, 30, x + espacio + 2, y - 14, 0);
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, 0, 90, 30, 30, x + espacio, y, 0);
					break;
				default:
					espacio -= 6;
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, 210, 90, 30, 30, x + espacio, y - 10, 0);
					espacio -= 8;
					i--;
					break;
				}
				i++;
			}
			else {
				al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, 210, 90, 30, 30, x + espacio, y - 14, 0);
			}
			break;
		case '.':
			espacio -= 5;
			al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, 240, 90, 30, 30, x + espacio, y, 0);
			break;
		case ',':
			espacio -= 5;
			al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, 270, 90, 30, 30, x + espacio, y, 0);
			break;
		case '\n':
			espacio = -22;
			renglon += 33;
			break;
		default://Revisar caracteres en general
			//Es un número
			if (isdigit(caracter)) {
				fila = 0;
				extra = 2;
				espacio -= 4;
			}
			//Es una letra. Asigna la fila en que se encuentra en el 'bitmap' para dibujarla
			else if (caracter >= 'A' && caracter <= 'J') {
				fila = 1;
				extra = 5;
			}
			else if (caracter >= 'K' && caracter <= 'T') {
				fila = 2;
				extra = 5;
			}
			else if (caracter >= 'U' && caracter <= 'Z') {
				fila = 3;
				extra = 5;
			}
			al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, color, (caracter + extra) % 10 * 30, fila * 30, 30, 30, x + espacio, y + renglon, 0);
			break;
		}
	}
}

void dibujar_titulo(Imagenes b, Cursor mouse) {
	al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.titulo_bitmap, 0, 0, 1280, 720, NORM_C, 640, 360, 640 + float(mouse.x) * 32 / RESOL_X - 16, 360 + float(mouse.y) * 18 / RESOL_Y - 9, 1.025, 1.025, 0, 0);
	al_draw_bitmap_region((ALLEGRO_BITMAP*)b.titulo_bitmap, 0, 720, 1280, 720, 0, 0, 0);

	//Dibujar botones
	if (mouse.x >= 836 && mouse.x < 1115 && mouse.y >= 198 && mouse.y < 368) {
		switch (mouse.estado) {
		default:mouse.estado = 1;	goto dibujar_boton_inicio;
		case 2:	al_draw_bitmap_region((ALLEGRO_BITMAP*)b.titulo_bitmap, 1560, 0, 279, 174, 836, 196, 0);	break;
		}
	}
	else {
		dibujar_boton_inicio:
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.titulo_bitmap, 1280, 0, 279, 174, 836, 196, 0);
	}

	if (mouse.x >= 857 && mouse.x < 1099 && mouse.y >= 368 && mouse.y < 443) {
		switch (mouse.estado) {
		default:mouse.estado = 1;	goto dibujar_boton_marcador;
		case 2:	al_draw_bitmap_region((ALLEGRO_BITMAP*)b.titulo_bitmap, 1523, 175, 242, 75, 857, 368, 0);	break;
		}
	}
	else {
		dibujar_boton_marcador:
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.titulo_bitmap, 1280, 175, 242, 75, 857, 368, 0);
	}

	dibujar_cursor(b, mouse);

	al_flip_display();
	void dibujar_selector(Imagenes, Cursor);
	void dibujar_selector(Imagenes, Cursor);
	void dibujar_selector(Imagenes, Cursor);
	void dibujar_selector(Imagenes, Cursor);
}

void dibujar_selector(Imagenes b, Cursor mouse) {
	ALLEGRO_COLOR color_aux;
	al_draw_bitmap((ALLEGRO_BITMAP*)b.fondo_casa_dia, -380, 0, 0);
	al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 0, 0, 940, 280, 15, 165, 0);

	//Dibujar botones
	//Botón iniciar ronda
	if (mouse.x >= 45 && mouse.x < 265 && mouse.y >= 445 && mouse.y < 575 && mouse.estado == 2) {
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 0, 410, 220, 130, 45, 445, 0);
	}
	else {
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 0, 280, 220, 130, 45, 445, 0);
	}
	//Botón Reiniciar selección
	if (mouse.x >= 270 && mouse.x < 490 && mouse.y >= 445 && mouse.y < 575 && mouse.estado == 2) {
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 220, 410, 220, 130, 270, 445, 0);
	}
	else {
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 220, 280, 220, 130, 270, 445, 0);
	}
	//Botón Aleatorio
	if (mouse.x >= 495 && mouse.x < 620 && mouse.y >= 445 && mouse.y < 575 && mouse.estado == 2) {
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 440, 405, 125, 131, 495, 445, 0);
	}
	else {
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 440, 280, 125, 130, 495, 445, 0);
	}

	//Dibujar semillero
	for (int i{ 0 }; i < LIM_SEM; i++) {
		if (semillero[i].plant != -1) {
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, (semillero[i].plant - 1) * 110, 0, 110, 110, i * 110 + 150, 0, 0);
			dibujar_numero(COST_PLANTA[semillero[i].plant], i * 110 + 215, 74, NORM_C, b);
			if (mouse.x >= i * 110 + 150 && mouse.x < i * 110 + 260 && mouse.y <= 110) {
				mouse.estado = 1;
			}
		}
		else {
			al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, TRANS_C, 565, 280, 110, 110, i * 110 + 150, 0, 0);
		}
	}

	//Dibujar Semillas
	for (int i{}; i < CANT_PLANT; i++) {
		bool en_semillero{ false };
		color_aux = NORM_C;
		for (int j{}; j < pos_semillero; j++) {
			if (semillero[j].plant == i + 1) {
				color_aux = al_map_rgb(64, 64, 64);
				en_semillero = true;
			}
		}
		al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, color_aux, i * 110, 0, 110, 110, i * 110 + 45, 190, 0);
		dibujar_numero(COST_PLANTA[i + 1], i * 110 + 110, 264, color_aux, b);
		if (mouse.x >= i * 110 + 45 && mouse.x < i * 110 + 155 && mouse.y >= 190 && mouse.y < 300 && (pos_semillero < LIM_SEM || en_semillero)) {
			mouse.estado = 1;
		}
	}

	dibujar_cursor(b, mouse);

	al_flip_display();
}

void dibujar_tablero(Imagenes b, Cursor mouse) {
	ALLEGRO_COLOR color_aux;
	short sobre_dibujo{ 0 };
	bool enseniar_cursor{ true };

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
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, 254, 950, 46, 46, color_aux, 19, 27, ptr_part->x + ptr_part->mov_x + 23, pos_y + 23, 1, 1, ptr_part->angulo * .01745, 0);
				break;
			case 1: case 3://Cono
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, (1 - ptr_part->id / 2) * 62, 945, 62, 62, color_aux, 32, 32, ptr_part->x + ptr_part->mov_x + 32, pos_y + 32, 1, 1, ptr_part->angulo * .01745, 0);
				break;
			case 2: case 4://Cubo
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, (1 - ptr_part->id / 2) * 62 + 186, 945, 62, 62, color_aux, 32, 32, ptr_part->x + ptr_part->mov_x + 32, pos_y + 32, 1, 1, ptr_part->angulo * .01745, 0);
				break;
			case 5://Brazo
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, 309, 963, 58, 24, color_aux, 19, 27, ptr_part->x + ptr_part->mov_x + 29, pos_y + 12, 1, 1, ptr_part->angulo * .01745, 0);
				break;
			case 6://Pierna
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, 381, 964, 34, 44, color_aux, 17, 22, ptr_part->x + ptr_part->mov_x + 17, pos_y + 22, 1, 1, ptr_part->angulo * .01745, 0);
				break;
			case 7://Corbata
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, 367, 968, 12, 28, color_aux, 6, 14, ptr_part->x + ptr_part->mov_x + 6, pos_y + 14, 1, 1, ptr_part->angulo * .01745, 0);
				break;
			}
		}
	}

	//Dibujar plantas, proyectiles y zombies en el tablero
	for (int y{}; y < CAS_Y; y++) {
		Proyectil* ptr_proy = NULL, * ant_proy = NULL;
		Zombie* ptr_zomb = NULL;

		//Dibujar plantas
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

		//Dibujar zombies
		ptr_zomb = zombie[y];
		while (ptr_zomb->sig_zomb != NULL) {
			//Se recorre al final de la lista enlazada
			ptr_zomb = ptr_zomb->sig_zomb;
		}
		for (int i{}; i < cant_zombi[y]; i++) {
			short color{ 0 };
			short color_r{ 255 }, color_g{ 255 }, color_b{ 255 };
			short estado_zomb{ 0 };
			float gorro_anim{ 0 };

			//Animación de daño
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

			//Determinar color del zombie
			color_aux = al_map_rgb(color_r, color_g, color_b);

			//Cambiar al sprite de daño
			if (ptr_zomb->est_danio == 1 || ptr_zomb->est_danio == -1) {
				estado_zomb = 135;
			}

			//Se mueve al zombie
			mover_zombie(*ptr_zomb, y);

			//Se dibuja al zombie
			if (ptr_zomb->comiendo) {
				estado_zomb = estado_zomb == 135 ? 648 : 0;
				al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, animacion_zombie(*ptr_zomb) + estado_zomb, 675, 108, 135, (int)ptr_zomb->x, y * 100 + 120, 0);
				al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, int(ptr_zomb->animacion / LIM_F_ZOMB_COM) % 4 * 108, 338, 108, 67, (int)ptr_zomb->x, y * 100 + 120, 0);
				//Dibujar gorrito
				if (ptr_zomb->pv > 10 && ptr_zomb->id != 0) {
					switch (ptr_zomb->id) {
					case 1://Cono
						gorro_anim = int(ptr_zomb->animacion / LIM_F_ZOMB_COM) * .4;
						switch (int(ptr_zomb->animacion / LIM_F_ZOMB_COM) % 4) {
						case 0:			gorro_anim = 0;		break;
						case 1: case 3: gorro_anim = 0.6;	break;
						case 2:			gorro_anim = 1.4;	break;
						}
						al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, (ptr_zomb->est_danio - 2) * 62, 945, 62, 62, (int)ptr_zomb->x + 23, y * 100 + gorro_anim * 2 + 95, 0);
						break;
					case 2://Cubeta
						gorro_anim = int(ptr_zomb->animacion / LIM_F_ZOMB_COM) * .4;
						switch (int(ptr_zomb->animacion / LIM_F_ZOMB_COM) % 4) {
						case 0:			gorro_anim = 0;		break;
						case 1: case 3: gorro_anim = 0.6;	break;
						case 2:			gorro_anim = 1.6;	break;
						}
						al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, (ptr_zomb->est_danio - 2) * 62 + 124, 945, 62, 62, (int)ptr_zomb->x + 15, y * 100 + gorro_anim * 3 + 111, 0);
						break;
					}
				}
			}
			else {
				switch (ptr_zomb->est_danio) {
				default://Normal
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, animacion_zombie(*ptr_zomb), estado_zomb, 108, 135, (int)ptr_zomb->x, y * 100 + 120, 0);
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, animacion_zombie(*ptr_zomb), 270, 108, 67, (int)ptr_zomb->x, y * 100 + 120, 0);
					break;
				case -1:
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, animacion_zombie(*ptr_zomb), estado_zomb, 108, 135, (int)ptr_zomb->x, y * 100 + 120, 0);
					break;
				case -2: case -3:
					if (ptr_zomb->est_danio == -3 && ptr_zomb->tiemp > 24) {
						float trans = 1.0 - (ptr_zomb->tiemp - 24) * .071;
						color_aux = al_map_rgba(color_r * trans, color_g * trans, color_b * trans, 255 * trans);
					}
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, animacion_zombie(*ptr_zomb), 405, 108, 135, (int)ptr_zomb->x, y * 100 + 120, 0);
					break;
				case -4: case -5:
					if (ptr_zomb->tiemp > 24) {
						float trans = 1.0 - (ptr_zomb->tiemp - 24) * .071;
						color_aux = al_map_rgba(color_r * trans, color_g * trans, color_b * trans, 255 * trans);
					}
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, animacion_zombie(*ptr_zomb), 540, 108, 135, (int)ptr_zomb->x, y * 100 + 120, 0);
					break;
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
						al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, (ptr_zomb->est_danio - 2) * 62, 945, 62, 62, (int)ptr_zomb->x + 23, y * 100 + gorro_anim * 2 + 89, 0);
						break;
					case 2://Cubeta
						gorro_anim = int(ptr_zomb->animacion / LIM_F_ZOMB) * .4;
						if (gorro_anim >= 3) {
							gorro_anim *= -1;
							gorro_anim += 5.5;
						}
						al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, (ptr_zomb->est_danio - 2) * 62 + 124, 945, 62, 62, (int)ptr_zomb->x + 15, y * 100 + gorro_anim * 3 + 105, 0);
						break;
					}
				}
			}

			//Reducir animación de daño
			if (ptr_zomb->anim_danio > 0) {
				ptr_zomb->anim_danio--;
			}

			//Siguiente nodo
			ptr_zomb = ptr_zomb->ant_zomb;
		}

		//Dibujar proyectiles
		ptr_proy = proyectil[y];
		for (int i{}; i < cant_proy[y]; i++) {
			ant_proy = ptr_proy;
			ptr_proy = ptr_proy->sig_proy;
			if (mover_proyectil(*ptr_proy, y)) {
				ptr_proy = ant_proy;
				i--;
				continue;
			}
			if (ptr_proy->tipo < 0) {
				if (!(frames % 6)) {
					ptr_proy->tiempo_ev++;
				}
				al_draw_bitmap_region((ALLEGRO_BITMAP*)b.guisante, ptr_proy->tiempo_ev * 28, ptr_proy->tipo * -28, 28, 28, ptr_proy->x, y * 100 + 189, 1);
			}
			else {
				al_draw_bitmap_region((ALLEGRO_BITMAP*)b.guisante, ptr_proy->tipo * 28, 0, 28, 28, ptr_proy->x, y * 100 + 189 + rand() % 3, 0);
			}
		}

		//Dibujar elementos sobre la pantalla
		for (int x{}; x < CAS_X && sobre_dibujo < anim_sobre_tablero; x++) {
			if (planta[x][y].pos) {
				//Explosión papapum
				if (planta[x][y].pos == 5 && planta[x][y].estado >= 3) {
					short trans = planta[x][y].tiemp > 20 ? 255 - (planta[x][y].tiemp - 20) * 42 : 255;
					color_aux = al_map_rgba(trans, trans, trans, trans);
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.explosion, color_aux, (int(planta[x][y].animacion / LIM_F_EXPLO)) * 200, 300, 200, 200, x * 100 + 145, y * 100 + 75, 0);
					if (planta[x][y].tiemp <= 12) {
						trans = planta[x][y].tiemp > 10 ? 255 - (planta[x][y].tiemp - 10) * 100 : 255;
						color_aux = al_map_rgba(trans, trans, trans, trans);
						al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.explosion, color_aux, 1800, 300, 300, 200, x * 100 + 93 + rand() % 6, y * 100 + 113 + rand() % 6, 0);
					}
					sobre_dibujo++;
				}
				//Mordida chomper
				else if (planta[x][y].pos == 7 && (planta[x][y].estado == 1 || planta[x][y].estado == 2)) {
					char color = planta[x][y].anim_danio > 0 ? 255 - planta[x][y].anim_danio * 7 : 255;
					if (planta[x][y].estado == 2) {
						unsigned char trans;
						if (planta[x][y].animacion > 6 * LIM_F_PLANT) {
							if ((planta[x][y].animacion - 6 * LIM_F_PLANT) * 60 < 255) {
								trans = 255 - (planta[x][y].animacion - 6 * LIM_F_PLANT) * 60;
							}
							else {
								trans = 0;
							}
						}
						else {
							trans = 255;
						}
						al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.explosion, al_map_rgba(trans * .7, trans * .5, trans, trans), 3200 + planta[x][y].tiemp % 2 * 200, 300, 200, 200, planta[x][y].esp.coord_zomb - 30, y * 100 + 90, planta[x][y].esp.coord_zomb % 2);
					}
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.chomper_anim, al_map_rgb(255, color, color), animacion_planta(x, y, true), 0, 180, 135, x * 100 + 195, y * 100 + 120, 0);
					sobre_dibujo++;
				}
			}
		}
	}

	//Dibujar explosión petacereza
	for (int y{ 0 }; y < CAS_Y && sobre_dibujo < anim_sobre_tablero; y++) {
		for (int x{ 0 }; x < CAS_X && sobre_dibujo < anim_sobre_tablero; x++) {
			if (planta[x][y].pos) {
				//Explosión petacereza
				if (planta[x][y].pos == 3 && planta[x][y].estado >= 1) {
					short trans = planta[x][y].animacion > 10 ? 255 - (planta[x][y].animacion - 10) * 100 : 255;
					color_aux = al_map_rgba(trans, trans, trans, trans);
					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.explosion, planta[x][y].animacion * 300, 0, 300, 300, x * 100 + 95, y * 100 + 65, 0);
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.explosion, color_aux, 2100, 300, 300, 200, x * 100 + 93 + rand() % 6, y * 100 + 113 + rand() % 6, 0);
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
			//siguiente nodo
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

	//Dibuja la interfaz de pala
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
	if (!soles_guard) {
		color_aux = al_map_rgb(255, 31, 95);
	}
	dibujar_numero(soles_guard - soles_guard_suma, 70, 90, color_aux, b);

	//Dibujar semillero
	for (int i{ 0 }; i < LIM_SEM; i++) {
		if (planta_elegida != semillero[i].plant) {
			color_aux = soles_guard >= COST_PLANTA[semillero[i].plant] ? al_map_rgb(255, 255, 255) : al_map_rgb(255, 31, 95);
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, (semillero[i].plant - 1) * 110, 0, 110, 110, i * 110 + 150, 0, 0);
			if (semillero[i].recarga > 0) {
				al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_recarga, al_map_rgba(90, 80, 90, 211), 0, 0, 110, (((float)semillero[i].recarga / REC_PLANTA[semillero[i].plant]) * 60 + 8), i * 110 + 150, 0, 0);
			}
			dibujar_numero(COST_PLANTA[semillero[i].plant], i * 110 + 215, 74, color_aux, b);
		}
		else {
			al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, al_map_rgb(64, 64, 64), (semillero[i].plant - 1) * 110, 0, 110, 110, i * 110 + 150, 0, 0);
			dibujar_numero(COST_PLANTA[semillero[i].plant], i * 110 + 215, 74, al_map_rgb(64, 64, 64), b);
		}
	}

	dibujar_texto((char*)"Puntuaci'on", 5, RESOL_Y - 85, NORM_C, b);
	dibujar_numero(oleada.puntos, 110, RESOL_Y - 50 , NORM_C, b);

	if (enseniar_cursor) {
		dibujar_cursor(b, mouse);
	}

	//Dibujar en ventana
	al_flip_display();
}

void dibujar_cursor(Imagenes b, Cursor mouse) {
	switch (pantalla) {
	case 1://Selector
		if (mouse.x >= 45 && mouse.x < 265 && mouse.y >= 445 && mouse.y < 575 && mouse.estado != 2) {
			mouse.estado = 1;
		}
		else if (mouse.x >= 270 && mouse.x < 490 && mouse.y >= 445 && mouse.y < 575 && mouse.estado != 2) {
			mouse.estado = 1;
		}
		else if (mouse.x >= 495 && mouse.x < 620 && mouse.y >= 445 && mouse.y < 575 && mouse.estado != 2) {
			mouse.estado = 1;
		}
		break;
	case 2://Juego
		//Dibujar Planta elegida/ Pala
		if (planta_elegida) {
			if (planta_elegida >= 1 && planta_elegida <= 8) {
				al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.plantas_dia, al_map_rgba(165, 165, 165, 200), (planta_elegida - 1) * 90, 0, 90, 90, mouse.x - 45, mouse.y - 45, 0);
				return;
			}
			else if (planta_elegida == -1) {
				al_draw_bitmap_region((ALLEGRO_BITMAP*)b.pala_interfaz, 110, 0, 110, 110, LIM_SEM * 110 + 150, 0, 0);
				al_draw_bitmap((ALLEGRO_BITMAP*)b.pala_cursor, mouse.x - 55, mouse.y - 55, 0);
				return;
			}
		}

		mouse.estado = 0;
		if (mouse.y < 110) {
			for (int i{ 0 }; i <= LIM_SEM; i++) {
				if (mouse.x >= 150 + i * 110 && mouse.x < 260 + i * 110) {
					if (i < LIM_SEM && semillero[i].recarga == 0) {
						mouse.estado = 1;
					}
					else if (i == LIM_SEM) {
						mouse.estado = 1;
					}
				}
			}
		}
		break;
	}

	al_draw_bitmap_region((ALLEGRO_BITMAP*)b.cursor_bitmap, mouse.estado * 70, 0, 70, 70, mouse.x - 20, mouse.y - 20, 0);
}