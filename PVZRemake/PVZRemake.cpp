/*----------LIBRERÍAS-----------------------------------------------------------------------------------------------------*/

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

/*----------CONSTANTES----------------------------------------------------------------------------------------------------*/

#define CAS_X 9
#define CAS_Y 5
#define POS_X 1
#define LIM_SEM 6

#define CAS_X_EXPL CAS_X + 2

#define LIM_F_PLANT 8
#define LIM_F_ZOMB 28
#define LIM_F_ZOMB_COM 12
#define LIM_F_EXPLO 4

#define FRAMES_INI 1800

#define CANT_PLANT 16
#define RESOL_X 1280
#define RESOL_Y 720
#define TIEMPO_ESC 60

#define CANT_REC 5

#define NORM_C al_map_rgb(255, 255, 255)
#define M_NORM_C(c) al_map_rgb(c, c, c)
#define TRANS_C al_map_rgba(204, 204, 204, 204)
#define M_TRANS_C(t) al_map_rgba(t, t, t, t)

#define al_draw al_draw_tinted_scaled_rotated_bitmap_region

enum OPC_POS { OPC_NULL, OPC_INICIAR, OPC_MARCADOR, OPC_REINICIAR = 2, OPC_ALEAT, OPC_TIEMPO, OPC_CONTINUAR = 1, OPC_SONIDO = 3, OPC_IR_INICIO, OPC_CLICK_PAUSA = 1 };
enum MOUSE_ESTADOS { MOUSE_EST_NORMAL, MOUSE_EST_APUNTA, MOUSE_EST_CLICK };

/*----------CONSTANTES-PLANTAS--------------------------------------------------------------------------------------------*/

const short COST_PLANTA	[CANT_PLANT + 1]{ 0, 100, 50, 150,  50,  25, 175, 150, 200,   0,  25,  75,  125,  75,  25,  75, 125 };
const short PV_PLANTA	[CANT_PLANT + 1]{ 0,  20, 20,  20, 350,  15,  20,  20,  20,  10,  20,  20,  275,   1,  20,  20,  20 };
const short REC_PLANTA	[CANT_PLANT + 1]{ 0,  60, 90, 600, 300, 300, 100,  90, 100,  80,  90, 100,  400, 0,  80, 800,1200 };
const short EXTR_SOL_DIA[CANT_PLANT + 1]{ 0,   0,  0,   0,   0,   0,   0,   0,   0,  50,  50,  50,    0,  50,  50,  75,  75 };

/*----------VARIABLES_GLOBALES--------------------------------------------------------------------------------------------*/

static short planta_elegida{ 0 }, semillero_elegido{ 0 };
static unsigned long long frames{ 0 }, frames_indep{ 0 };
static bool bucle{ true }, programa_corriendo{ true };

static short cont, soles_guard{ 50 }, soles_guard_suma{ 0 }, plantas_en_semillero[LIM_SEM];
static short cant_sol_tablero{ 0 }, cant_particulas[CAS_Y]{ 0,0,0,0,0 };
static short cant_zombi[CAS_Y]{ 0,0,0,0,0 }, cant_zombi_ignorar[CAS_Y]{ 0,0,0,0,0 }, cant_proy[CAS_Y], anim_sobre_tablero{ 0 };
static long cant_zombi_elim{ 0 }, cant_sol_recol{ 0 };

static short pantalla{ 0 }, pos_semillero{ 0 }, indice_nombre{ 0 }, tipo_patio{ 0 };

static bool fin_juego{ false }, pausa_total{ false }, pantalla_completa{ false }, nom_compl{ false }, foto{ false };
static short resol_x{ RESOL_X }, resol_y{ RESOL_Y };
static float tam_pant_x{ 1 }, tam_pant_y{ 1 };

/*----------ESTRUCTURAS---------------------------------------------------------------------------------------------------*/

struct Sol {
	short estado_act;
	short cant;
	short angulo;
	short tiemp;
	float x, y, tam;
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
	float x, y;
	Proyectil* ant_proy, * sig_proy;
	short tipo;
	short cas_x_ini;
	union {
		float trasl_x;		//Humo humoseta
	}esp;
}	static* proyectil[CAS_Y];

struct Semillero {
	short plant, recarga;
	bool aleat;
}	static  semillero[LIM_SEM];

struct Zombie {
	short id;
	short pv;
	short tiemp;
	short tiemp_muert;
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
	short dx, dy;
	//Especialidades de cada planta
	union {
		short mordidas;		//Nuez
		short coord_zomb;	//Chomper
		short repet;		//Repetidora
		bool zombie_detect;	//Humoseta
		short crecimiento;	//Seta Solar
	}esp;
}		static  planta[CAS_X][CAS_Y];

struct Danio_area {
	short danio;
	short tipo_area;
}	static explosion_tablero[CAS_X_EXPL][CAS_Y];

struct Ronda {
	short zombie_ataq;
	short tiemp_no_zomb;
	int puntos_sum;
	long long puntos;
	short dificultad, lim_dificultad, tiempo_oleada, ritmo_nivel, lim_tiempo_oleada,tiempo;
}		static	oleada;

struct Imagenes {
	//Imágenes generales
	void* cursor_bitmap;
	void* fuente_bitmap;
	void* arbustos_transicion;
	void* fondo_casa_dia;
	void* fondo_casa_noche;
	void* enfoque_oscuro;

	//Imágenes pantalla título
	void* titulo_bitmap;
	void* notas_bitmap;

	//Imágenes selector
	void* selector_hud;
	void* texto_inicio_part;

	//Imágenes juego normal
	void* plantas_dia;
	void* plantas_noche;
	void* chomper_anim;
	void* semillero_bitmap;
	void* hud_juego_bitmap;
	void* hud_pausa_bitmap;
	void* sol_bitmap;
	void* explosion;
	void* guisante;
	void* zombie_bitmap;
	void* pantalla_blanca;
	void* hipno_fondo;

	//Buffers
	void* sombra_buffer;
	void* objetos_buffer;

	//Imágenes fin del juego
	void* fondo_plantas;

	//ETCÉTERA
	struct {
		void* display;
		void* buffer;
		void* hud_buffer;
	}al;
}	static b;

struct Particula {
	/*ID - Tipo
	0 - Cabeza
	1 - Cono (3 para intacto)
	2 - Cubeta (4 para intacto)
	5 - Brazo
	6 - Corbata
	7 - Pierna
	*/
	short id, fila;
	short x, y;
	short mov_x, impulso;
	short angulo;
	short estado;
	short tiemp;
	bool dir;
	Particula *ant_part, *sig_part;
}	static* particulas[CAS_Y];

struct Cursor {
	short x, y, estado, tiemp_esc;
	short opc_sel;
}		static mouse;

struct Efectos_esp {
	short tipo;
	union {
		//ID 1 y 2
		struct {
			short tiemp, tiemp_max;
			float transp;
			unsigned char r, g, b;
		}pant;
		//ID 3
		struct {
			short x;
			short y;
			short tiemp;
			short aleatorio;
		}expl;
	};
	Efectos_esp* ant, * sig;
}	static *vfx;

struct Transicion {
	short id;
	short pantalla_ir;
	float y_global;
	bool finalizado;
	union {
		//ID 1
		struct ARB{
			short y;
		}arbustos;
		//ID 2
		struct JUEGO{
			float x, hud_x, hud_y;
			short tiemp;
			short text_act;
		}juego_ini;
		//ID 3
		struct MANO_Z{
			short tiemp;
			short y, arbust_y;
		}mano;
		//ID 4
		struct SEMILLA {
			float x, y, tam, tam_letr;
			short tiemp, fila;
			short temblor;
		}muerte;
		//ID 5
		struct GAME_OVER {
			float cant_zombi_elim_anim;
			float cant_sol_recol_anim;
			double puntuacion_anim;
			short tiemp;
		}fin;
	};
}	static tr;

struct Record{
	char nombre[3];
	long long puntos;
	unsigned long long tiempo;
}		static guardar, *tabla;

/*----------PROTOTIPOS----------------------------------------------------------------------------------------------------*/

//INPUT
void registrar_teclas(ALLEGRO_EVENT, bool);
void registrar_mouse(ALLEGRO_EVENT, bool);
void posicion_cursor(ALLEGRO_EVENT);

//PLANTAS
void eliminar_planta(Planta&);
void funcion_planta(short, short);
void generar_planta(short, Planta&);
void plantar_planta();
void seleccionar_planta(short);
short animacion_planta(short, short, bool);

//SOL
bool funcion_sol(Sol&, short);
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
void generar_particula(short, short, short, bool, short);
void rotacion_particula_x(Particula&);
float animacion_cabeza_part(Particula&);
void oleadas_zombie();

//EFECTOS
bool funcion_efecto(Efectos_esp&);
void vfx_pantalla(short, short, short, float, short);
void vfx_destello(short, short, short, float, short);
void vfx_explosion_petacereza(short, short);
void vfx_explosion_petaseta(short, short);
Efectos_esp* generar_efecto_fin();
Efectos_esp* generar_efecto_ini();

//DIBUJADO
void dibujar_numero(long long, float, float, ALLEGRO_COLOR);
void dibujar_texto(char*, short, short, ALLEGRO_COLOR);
void dibujar_record(float, float);

void dibujo_planta_sombra(Planta&, short, short);

void dibujar_titulo();
void dibujar_selector();
void dibujar_tablero();
void dibujar_cursor();
void dibujar_fin_juego();

//EXTRA
void funcion_semillero(); 
void cambiar_pantalla_ventana();
void cambiar_pantalla_completa();
void finalizar_ejecucion();
void reescalar_pantalla();

void guardar_record(Record);
void cargar_record();
void guardar_captura();

//INICIAR/ TERMINAR JUEGO
void inicializar_titulo();
void finalizar_titulo(); 

void inicializar_selector();
void finalizar_selector();

void inicializar_juego();
void finalizar_juego();

void inicializar_fin_juego();
void finalizar_fin_juego();

/*----------FUNCIÓN-MAIN--------------------------------------------------------------------------------------------------*/

int main() {
	srand(time(0));
	bool cursor_org{ true };
	int pantalla_org;

	tr.y_global = 0;
	mouse.estado = 0;
	mouse.x = RESOL_X / 2;
	mouse.y = RESOL_Y / 2;
	mouse.opc_sel = OPC_NULL;
	mouse.tiemp_esc = 0;

	vfx = new Efectos_esp;
	vfx-> ant = vfx->sig = NULL;
	vfx->tipo = -1;

	std::cout << "INICIO DE PROGRAMA" << std::endl;

	if (!al_init())				return -1;
	if (!al_init_image_addon())	return -1;
	if (!al_install_keyboard())	return -1;
	if (!al_install_mouse())	return -1;
	if (!al_install_audio())	return -1;
	if (!al_init_acodec_addon())return -1;
	if (!al_reserve_samples(16))return -1;

	cambiar_pantalla_ventana();

	b.al.display = al_create_display(resol_x, resol_y);
	al_set_window_title((ALLEGRO_DISPLAY*)b.al.display, "Plantas Contra Zombies Remake");
	al_set_display_icon((ALLEGRO_DISPLAY*)b.al.display, al_load_bitmap("Sprites/Icon.png"));
	al_hide_mouse_cursor((ALLEGRO_DISPLAY*)b.al.display);

	ALLEGRO_EVENT_QUEUE* cola_eventos = al_create_event_queue();
	ALLEGRO_TIMER* tiempo = al_create_timer(1.0 / 60);
	ALLEGRO_EVENT eventos;
	ALLEGRO_COLOR color_aux = NORM_C;

	b.cursor_bitmap = al_load_bitmap("Sprites/Extra/Cursor_Bitmap.png");
	b.fuente_bitmap = al_load_bitmap("Sprites/Extra/Font_Bitmap.png");
	b.arbustos_transicion = al_load_bitmap("Sprites/Transition_Bushes.png");
	b.fondo_plantas = al_load_bitmap("Sprites/Plants_Background.png");

	al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
	b.al.buffer = al_create_bitmap(RESOL_X, RESOL_Y);
	
	tr.id = -1;

	//Registrar entradas. Teclado, mouse y tiempo
	al_register_event_source(cola_eventos, al_get_keyboard_event_source());
	al_register_event_source(cola_eventos, al_get_mouse_event_source());
	al_register_event_source(cola_eventos, al_get_timer_event_source(tiempo));

	//Iniciar el tiempo
	al_start_timer(tiempo);

	while (programa_corriendo) {
		switch (pantalla) {
		case 0: //Título
			inicializar_titulo();

			//bucle título
			while (bucle) {
				pantalla_org = pantalla;
				al_wait_for_event(cola_eventos, &eventos);
				posicion_cursor(eventos);
				switch (eventos.type) {
					//Dibuja
					case ALLEGRO_EVENT_TIMER:
						if (mouse.tiemp_esc != 0) {
							finalizar_ejecucion();
						}
						dibujar_titulo();
						break;

						//Registrar teclas
					case ALLEGRO_EVENT_KEY_DOWN:
						registrar_teclas(eventos, false);
						break;
					case ALLEGRO_EVENT_KEY_UP:
						registrar_teclas(eventos, true);
						break;

						//Registrar botones mouse
					case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
						registrar_mouse(eventos, true);
						break;
					case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
						registrar_mouse(eventos, false);
						break;
				}//Switch eventos.type
				if (tr.finalizado && tr.id == 3) {
					short coordenada_y = tr.mano.arbust_y;
					pantalla = 1;
					tr.id = 1;
					tr.arbustos.y = coordenada_y - RESOL_Y / 30;
				}
				if (pantalla != pantalla_org) {
					bucle = false;
				}
			}//While Bucle

			finalizar_titulo();
			break;

		case 1: //Selector
			inicializar_selector();

			//Bucle selector
			while (bucle) {
				pantalla_org = pantalla;
				al_wait_for_event(cola_eventos, &eventos);
				posicion_cursor(eventos);
				switch (eventos.type) {
					//Dibuja
				case ALLEGRO_EVENT_TIMER:
					if (mouse.tiemp_esc != 0) {
						finalizar_ejecucion();
					}
					dibujar_selector();
					break;

					//Registrar teclas
				case ALLEGRO_EVENT_KEY_DOWN:
					registrar_teclas(eventos, false);
					break;
				case ALLEGRO_EVENT_KEY_UP:
					registrar_teclas(eventos, true);
					break;


					//Registrar botones mouse
				case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
					registrar_mouse(eventos, true);
					break;
				case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
					registrar_mouse(eventos, false);
					break;
				}//Switch eventos.type
				if (tr.finalizado == true && tr.id == 2) {
					pantalla = 2;
				}
				if (pantalla != pantalla_org) {
					bucle = false;
				}
			}//While Bucle

			finalizar_selector();
			break;

		case 2: //Juego
			inicializar_juego();

			//Bucle Juego
			while (bucle) {
				pantalla_org = pantalla;
				al_wait_for_event(cola_eventos, &eventos);
				posicion_cursor(eventos);
				switch (eventos.type) {
					//El juego avanza
				case ALLEGRO_EVENT_TIMER:
					if (mouse.tiemp_esc != 0) {
						finalizar_ejecucion();
					}
					//Revisar funciones
					if (!pausa_total) {
						oleadas_zombie();
					}
					if (!(frames % 6) && !pausa_total) {
						funcion_semillero();
						//Revisar soles
						if (cant_sol_tablero) {
							Sol* ptr_sol = NULL, * anterior_sol = NULL;
							ptr_sol = sol_tablero;
							for (int i{}; i < cant_sol_tablero; i++) {
								anterior_sol = ptr_sol;
								ptr_sol = ptr_sol->sig_sol;
								if (funcion_sol(*ptr_sol, i)) {
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
								cant_zombi_ignorar[i] = 0;
								for (int j{}; j < cant_zombi[i]; j++) {
									anterior_zomb = ptr_zomb;
									ptr_zomb = ptr_zomb->sig_zomb;
									if (funcion_zombie(*ptr_zomb, i)) {
										ptr_zomb = anterior_zomb;
										j--;
									}
									else if (ptr_zomb->est_danio < 0) {
										cant_zombi_ignorar[i]++;
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
					if (!(frames % 800) && !pausa_total && tipo_patio % 2 == 0) {
						generar_sol_recolect(rand() % (RESOL_X - 500) + 300, 0, 25);
					}

					dibujar_tablero();
					if (!pausa_total) {
						frames++;
					}
					frames_indep++;
					break;

					//Registrar teclas
				case ALLEGRO_EVENT_KEY_DOWN:
					registrar_teclas(eventos, false);
					break;
				case ALLEGRO_EVENT_KEY_UP:
					registrar_teclas(eventos, true);
					break;


					//Registrar botones mouse
				case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
					registrar_mouse(eventos, true);
					break;
				case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
					registrar_mouse(eventos, false);
					break;

				}
				if (tr.finalizado && tr.id == 4) {
					tr.muerte.tiemp = 0;
					tr.muerte.y = 300;
					pantalla = 3;
				}
				
				//Switch eventos.type
				if (pantalla != pantalla_org) {
					bucle = false;
				}
			}//While Bucle

			finalizar_juego();
			break;
		case 3://Game over
			inicializar_fin_juego();

			while (bucle) {
				pantalla_org = pantalla;
				al_wait_for_event(cola_eventos, &eventos);
				posicion_cursor(eventos);
				switch (eventos.type) {
					//Dibuja
				case ALLEGRO_EVENT_TIMER:
					if (mouse.tiemp_esc != 0) {
						finalizar_ejecucion();
					}
					dibujar_fin_juego();
					frames++;
					break;

					//Registrar teclas
				case ALLEGRO_EVENT_KEY_DOWN:
					registrar_teclas(eventos, false);
					break;
				case ALLEGRO_EVENT_KEY_UP:
					registrar_teclas(eventos, true);
					break;


					//Registrar botones mouse
				case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
					registrar_mouse(eventos, true);
					break;
				case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
					registrar_mouse(eventos, false);
					break;
				}//Switch eventos.type
				if (tr.id == 1 && tr.finalizado) {
					tr.arbustos.y -= RESOL_Y / 30;
					pantalla = 0;
				}
				if (pantalla != pantalla_org) {
					bucle = false;
				}
			}

			finalizar_fin_juego();
			break;
		}//Switch Pantalla

	}//El programa se termina

	//LIBERACIÓN DE ESPACIO
	if (vfx->sig) {
		Efectos_esp* aux, * act;
		short i{ 0 };
		act = vfx;
		while (act) {
			aux = act->sig;
			delete act;
			act = aux;
			i++;
		}
		std::cout << "SE ELIMINARON " << i << " VFX" << std::endl;
	}
	else {
		delete vfx;
	}

	al_destroy_timer(tiempo);
	al_destroy_event_queue(cola_eventos);
	al_destroy_display((ALLEGRO_DISPLAY*)b.al.display);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.cursor_bitmap);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.fuente_bitmap);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.arbustos_transicion);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.fondo_plantas);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.al.buffer);
}

/*----------FUNCIONES-INPUT-----------------------------------------------------------------------------------------------*/

void registrar_mouse(ALLEGRO_EVENT evento, bool presionado) {
	bool encontrado;
	switch (pantalla) {
	case 0://Pantalla de título
		switch (evento.mouse.button) {
		case ALLEGRO_MOUSE_BUTTON_LEFT:
			if (presionado && tr.id == -1) {
				mouse.opc_sel = OPC_NULL;

				if (mouse.x >= 836 && mouse.x < 1115 && mouse.y >= 198 && mouse.y < 368) {
					mouse.opc_sel = OPC_INICIAR;
				}

				if (mouse.x >= 857 && mouse.x < 1099 && mouse.y >= 368 && mouse.y < 443) {
					mouse.opc_sel = OPC_MARCADOR;
				}

				mouse.estado = 2;
			}
			else if (tr.id == -1) {
				if (mouse.x >= 836 && mouse.x < 1115 && mouse.y >= 198 && mouse.y < 368 && mouse.estado == 2 && mouse.opc_sel == OPC_INICIAR) {
					//Iniciar transición al semillero
					tr.id = 3;
					tr.finalizado = false;
					tr.mano.tiemp = 0;
					tr.mano.y = RESOL_Y;
					tr.mano.arbust_y = RESOL_Y;
					if (tr.y_global > 0) {
						tr.y_global = -565;
					}
				}

				if (mouse.x >= 857 && mouse.x < 1099 && mouse.y >= 368 && mouse.y < 443 && mouse.estado == 2 && mouse.opc_sel == OPC_MARCADOR) {
					if (tr.y_global <= 0) {
						tr.y_global = 565;
					}
					else {
						tr.y_global = -565;
					}
				}
				mouse.estado = 0;
			}
			if (!presionado) {
				mouse.estado = 0;
			}
		}
		break;
	case 1://Selector
		switch (evento.mouse.button) {
		case ALLEGRO_MOUSE_BUTTON_LEFT:
			if (presionado && tr.id == -1) {
				mouse.opc_sel = OPC_NULL;

				//Buscar ratón en la selección de semillas
				if (mouse.y <= 110) {
					for (int i{}; i < pos_semillero; i++) {
						if (mouse.x >= i * 110 + 150 && mouse.x < i * 110 + 260 && pos_semillero) {
							for (int k{ i }; k <= pos_semillero; k++) {
								if (k + 1 < LIM_SEM) {
									semillero[k] = semillero[k + 1];
								}
								else {
									semillero[k].plant = -2;
								}
							}
							pos_semillero--;
						}
					}
				}
				//buscar ratón en el semillero
				if (mouse.y >= 190 && mouse.y < 300) {
					//revisa en la primera fila de plantas
					for (int i{}; i < 8; i++) {
						if (mouse.x >= i * 110 + 45 && mouse.x < i * 110 + 155) {
							encontrado = false;
							//Revisa si la planta a elegir se encuentra en el semillero del jugador
							for (int j{}; j < pos_semillero; j++) {
								if (semillero[j].plant == i + 1) {
									//Lo está, por lo que elimina la planta del semillero
									encontrado = true;
									for (int k{ j }; k < pos_semillero; k++) {
										if (k + 1 < LIM_SEM) {
											semillero[k] = semillero[k + 1];
										}
										else {
											semillero[k].plant = -2;
										}
									}
									pos_semillero--;
								}
							}
							//Si no se encontró, se añade al semillero
							if (!encontrado && pos_semillero < LIM_SEM) {
								semillero[pos_semillero].plant = i + 1;
								pos_semillero++;
							}
						}
					}
				}
				if (mouse.y >= 300 && mouse.y < 410) {
					//revisa en la segunda fila de plantas
					for (int i{}; i < 8; i++) {
						if (mouse.x >= i * 110 + 45 && mouse.x < i * 110 + 155) {
							encontrado = false;
							//Revisa si la planta a elegir se encuentra en el semillero del jugador
							for (int j{}; j < pos_semillero; j++) {
								if (semillero[j].plant == i + 9) {
									//Lo está, por lo que elimina la planta del semillero
									encontrado = true;
									for (int k{ j }; k < pos_semillero; k++) {
										if (k + 1 < LIM_SEM) {
											semillero[k] = semillero[k + 1];
										}
										else {
											semillero[k].plant = -2;
										}
									}
									pos_semillero--;
								}
							}
							//Si no se encontró, se añade al semillero
							if (!encontrado && pos_semillero < LIM_SEM) {
								semillero[pos_semillero].plant = i + 9;
								pos_semillero++;
							}
						}
					}
				}
				//Buscar ratón en el Botón de inicio ronda
				if (mouse.x >= 45 && mouse.x < 265 && mouse.y >= 445 && mouse.y < 575) {
					mouse.opc_sel = OPC_INICIAR;
				}//Buscar ratón en el Botón reinicio
				if (mouse.x >= 270 && mouse.x < 490 && mouse.y >= 445 && mouse.y < 575) {
					mouse.opc_sel = OPC_REINICIAR;
				}//buscar ratón en el Botón aleatorio
				if (mouse.x >= 495 && mouse.x < 620 && mouse.y >= 445 && mouse.y < 575) {
					mouse.opc_sel = OPC_ALEAT;
				}//buscar ratón en el Botón de tiempo
				if (mouse.x >= 630 && mouse.x < 755 && mouse.y >= 445 && mouse.y < 575) {
					mouse.opc_sel = OPC_TIEMPO;
				}
				mouse.estado = 2;
			}
			else if (tr.id == -1) {
				//Buscar ratón en el Botón de inicio ronda
				if (mouse.x >= 45 && mouse.x < 265 && mouse.y >= 445 && mouse.y < 575 && mouse.opc_sel == OPC_INICIAR) {
					soles_guard_suma = 50;
					tr.finalizado = false;
					tr.id = 2;
					tr.juego_ini.hud_x = 1;
					tr.juego_ini.hud_y = 110;
					tr.juego_ini.x = 1;
					tr.juego_ini.tiemp = 0;
					tr.juego_ini.text_act = -1;
				}//Buscar ratón en el Botón reinicio
				if (mouse.x >= 270 && mouse.x < 490 && mouse.y >= 445 && mouse.y < 575 && mouse.opc_sel == OPC_REINICIAR) {
					for (int i{}; i < LIM_SEM; i++) {
						semillero[i].plant = -2;
					}
					pos_semillero = 0;
				}//buscar ratón en el Botón aleatorio
				if (mouse.x >= 495 && mouse.x < 620 && mouse.y >= 445 && mouse.y < 575 && mouse.opc_sel == OPC_ALEAT) {
					for (int i{}, aleat; i < LIM_SEM; i++) {
						aleat = rand() % CANT_PLANT + 1;
						for (int j{}; j <= i; j++) {
							if (aleat == semillero[j].plant) {
								aleat = rand() % CANT_PLANT + 1;
								j = -1;
							}
						}
						semillero[i].plant = aleat;
						plantas_en_semillero[i] = aleat;
					}
					pos_semillero = LIM_SEM;
				}//buscar ratón en el Botón de tiempo
				if (mouse.x >= 630 && mouse.x < 755 && mouse.y >= 445 && mouse.y < 575 && mouse.opc_sel == OPC_TIEMPO) {
					tr.y_global = 255;
					tipo_patio = tipo_patio % 2 ? tipo_patio - 1 : tipo_patio + 1;
				}
				mouse.estado = 0;
			}
			if (!presionado) {
				mouse.estado = 0;
			}
		}
		break;
	case 2://En el juego
		switch (evento.mouse.button) {
		case ALLEGRO_MOUSE_BUTTON_LEFT:
			if (presionado) {
				mouse.estado = MOUSE_EST_CLICK;
				//Botón pausa
				if (mouse.x >= RESOL_X - 110 && mouse.x < RESOL_X && mouse.y > 0 && mouse.y < 110) {
					pausa_total = fin_juego ? 1 : !pausa_total;
				}
				//Juego Normal
				if (!pausa_total) {
					if (mouse.y >= 160) {
						plantar_planta();
					}
					else if (mouse.y <= 110) {
						seleccionar_planta(0);
					}
				}
				//Menú pausa
				else {
					//Botón Continuar
					if (mouse.x >= RESOL_X / 2 - 101 && mouse.x < RESOL_X / 2 + 101 && mouse.y >= 280 && mouse.y < 365) {
						mouse.opc_sel = OPC_CONTINUAR;
					}
					//Botón Reiniciar
					if (mouse.x >= RESOL_X / 2 - 101 && mouse.x < RESOL_X / 2 + 101 && mouse.y >= 375 && mouse.y < 460) {
						mouse.opc_sel = OPC_REINICIAR;
					}
					//Botón volumen
					if (mouse.x >= RESOL_X / 2 - 105 && mouse.x < RESOL_X / 2 - 5 && mouse.y >= 460 && mouse.y < 555) {
						mouse.opc_sel = OPC_SONIDO;
					}
					//Botón casa
					if (mouse.x >= RESOL_X / 2 && mouse.x < RESOL_X / 2 + 100 && mouse.y >= 460 && mouse.y < 555) {
						mouse.opc_sel = OPC_IR_INICIO;
					}
				}
			}
			else {
				mouse.estado = MOUSE_EST_NORMAL;
				if (pausa_total) {
					//Botón Continuar
					if (mouse.x >= RESOL_X / 2 - 101 && mouse.x < RESOL_X / 2 + 101 && mouse.y >= 280 && mouse.y < 365 && mouse.opc_sel == OPC_CONTINUAR) {
						pausa_total = fin_juego ? 1 : !pausa_total;
					}
					//Botón Reiniciar
					if (mouse.x >= RESOL_X / 2 - 101 && mouse.x < RESOL_X / 2 + 101 && mouse.y >= 375 && mouse.y < 460 && mouse.opc_sel == OPC_REINICIAR) {
						pantalla = 1;
						tr.id = -1;
					}
					//Botón casa
					if (mouse.x >= RESOL_X / 2 && mouse.x < RESOL_X / 2 + 100 && mouse.y >= 460 && mouse.y < 555 && mouse.opc_sel == OPC_IR_INICIO) {
						pantalla = 0;
						tr.id = -1;
					}
				}
				mouse.opc_sel = OPC_NULL;
			}
			break;
		}
		break;
	}
}

void registrar_teclas(ALLEGRO_EVENT teclado, bool manten_presionado) {
	char letra;
	if (!manten_presionado) {
		switch (pantalla) {
		case 2://Tablero
			switch (teclado.keyboard.keycode) {
			default://teclas extra
				if (teclado.keyboard.keycode >= 28 && teclado.keyboard.keycode <= 36 && !pausa_total) {
					seleccionar_planta(teclado.keyboard.keycode - 27);
				}
				break;
			case ALLEGRO_KEY_L:
				if (!pausa_total)
					seleccionar_planta(-1);
				break;
			case ALLEGRO_KEY_Z: generar_zombie(rand() % CAS_Y, 3); break;
			case ALLEGRO_KEY_S: soles_guard += 1000; soles_guard_suma += 1000; break;
			case ALLEGRO_KEY_ENTER:
				pausa_total = fin_juego ? 1 : !pausa_total;
				break;
			case ALLEGRO_KEY_F: cambiar_pantalla_completa(); break;
			case ALLEGRO_KEY_F5:	foto = true; break;
			}
			break;
		case 3://Fin del juego
			switch (teclado.keyboard.keycode) {
			default://teclas extra
				if (tr.fin.tiemp > 420 && tr.id == 5) {
					if (teclado.keyboard.keycode >= ALLEGRO_KEY_A && teclado.keyboard.keycode <= ALLEGRO_KEY_Z) {
						letra = (char)teclado.keyboard.keycode + 64;
						guardar.nombre[indice_nombre % 3] = letra;
						indice_nombre++;
						if (indice_nombre >= 3) {
							nom_compl = true;
						}
						std::cout << letra << std::endl;
					}
				}
				break;
			case ALLEGRO_KEY_ENTER:
				if (nom_compl) {
					tr.id = 1;
					tr.finalizado = false;
					tr.arbustos.y = RESOL_Y;
				}
				break;
			case ALLEGRO_KEY_F: cambiar_pantalla_completa(); break;
			case ALLEGRO_KEY_F5:	foto = true; break;
			}
			break;
		default:
			switch (teclado.keyboard.keycode) {
			case ALLEGRO_KEY_F: cambiar_pantalla_completa(); break;
			case ALLEGRO_KEY_F5:	foto = true; break;
			}
			break;
		}
		if (teclado.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
			mouse.tiemp_esc = 1;
		}
	}
	else {
		if (teclado.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
			if (mouse.tiemp_esc <= 6) {
				bucle = false;
				programa_corriendo = false;
			}
			else {
				mouse.tiemp_esc = 0;
			}
		}
	}
}

void posicion_cursor(ALLEGRO_EVENT cursor) {
	if (cursor.type == ALLEGRO_EVENT_MOUSE_AXES) {
		mouse.x = cursor.mouse.x / tam_pant_x;
		mouse.y = cursor.mouse.y / tam_pant_y;
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
			if (planta[x][y].estado == 0 && cant_zombi[y] - cant_zombi_ignorar[y] > 0 && planta[x][y].tiemp >= 12 + rand() % 4) {
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
					planta[x][y].animacion = rand() % (15 * LIM_F_PLANT - 1);
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
					vfx_explosion_petacereza(x * 100 + 95, y * 100 + 65);
					for (int sx{ -1 }; sx <= 1; sx++) {
						if (x + sx >= 0 && x + sx < CAS_X_EXPL) {
							for (int sy{ -1 }; sy <= 1; sy++) {
								if (y + sy >= 0 && y + sy < CAS_Y) {
									explosion_tablero[x + sx][y + sy].danio += 200;
									explosion_tablero[x + sx][y + sy].tipo_area = 1;
								}
							}
						}
					}
				}
				break;
			case 1://Evita hacer daño doble
				for (int sx{ -1 }; sx <= 1; sx++) {
					if (x + sx >= 0 && x + sx < CAS_X_EXPL) {
						for (int sy{ -1 }; sy <= 1; sy++) {
							if (y + sy >= 0 && y + sy < CAS_Y) {
								explosion_tablero[x + sx][y + sy].danio -= 200;
								explosion_tablero[x + sx][y + sy].tipo_area = 0;
							}
						}
					}
				}
				eliminar_planta(planta[x][y]);
				break;
			}
		case 4: case 12: //NUEZ y CALABAZA
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
			if (planta[x][y].estado == 0 && cant_zombi[y] - cant_zombi_ignorar[y] > 0 && planta[x][y].tiemp >= 12 + rand() % 5) {
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
			if (planta[x][y].estado == 0 && cant_zombi[y] - cant_zombi_ignorar[y] > 0 && planta[x][y].tiemp >= 12 + rand() % 5) {
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
		case 9://HONGUITO DESESPORADO
			if (planta[x][y].estado == 0 && planta[x][y].esp.zombie_detect && planta[x][y].tiemp >= 12 + rand() % 4) {
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
					planta[x][y].esp.zombie_detect = false;
				}
			}
			break;
		case 10://HONGO SOLAR
			if (planta[x][y].estado == 2 && planta[x][y].animacion >= 5 * LIM_F_PLANT - 1) {
				planta[x][y].estado = 3;
				planta[x][y].animacion = 0;
			}
			if (planta[x][y].estado == 1 || planta[x][y].estado == 4) {
				//Generar soles
				if ((planta[x][y].tiemp >= 10 && planta[x][y].estado == 1) ||
					(planta[x][y].tiemp >= 12 && planta[x][y].estado == 4)) {
					int sol_max, sol_gen{ 0 };
					planta[x][y].animacion = 8 * LIM_F_PLANT + rand() % LIM_F_PLANT;
					planta[x][y].tiemp = 0;
					if (planta[x][y].estado == 1) {
						sol_max = 3;
						planta[x][y].estado = 0;
						planta[x][y].esp.crecimiento--;
						while (sol_max > 0) {
							sol_gen = rand() % sol_max + 1;
							sol_max -= sol_gen;
							generar_sol_recolect(x * 100 + 195, y * 100 + 165, sol_gen * 5);
						}
						if (planta[x][y].esp.crecimiento <= 0) {
							planta[x][y].estado = 2;
							planta[x][y].animacion = 0;
						}
					}
					else if (planta[x][y].estado == 4) {
						sol_max = 5;
						planta[x][y].estado = 3;
						while (sol_max > 0) {
							if (sol_max > 3) {
								sol_gen = rand() % 3 + 1;
							}
							else {
								sol_gen = rand() % sol_max + 1;
							}
							sol_max -= sol_gen;
							generar_sol_recolect(x * 100 + 195, y * 100 + 165, sol_gen * 5);
						}
					}
				}
			}
			else if (planta[x][y].estado == 0 || planta[x][y].estado == 3) {
				short tiemp_espera{ 10 };	//Espera personalizada para cada uno de los estados
				//cambio de estado: soles
				if (planta[x][y].estado == 0) {
					tiemp_espera = 110;
				}
				else {
					tiemp_espera = 120;
				}
				if (planta[x][y].tiemp >= tiemp_espera + rand() % 40) {
					planta[x][y].animacion = 0;
					planta[x][y].tiemp = 0;
					if (planta[x][y].estado == 0) {
						planta[x][y].estado = 1;
					}
					else if (planta[x][y].estado == 3) {
						planta[x][y].estado = 4;
					}
					std::cout << "SOL GENERANDO" << std::endl;
				}
			}
			break;
		case 11://Humoseta
			if (planta[x][y].estado == 0 && planta[x][y].tiemp >= 4 + rand() % 3 && planta[x][y].esp.zombie_detect) {
				planta[x][y].estado = 1;
				planta[x][y].animacion = 0;
				planta[x][y].tiemp = 0;
			}
			if (planta[x][y].estado == 1) {
				if (planta[x][y].animacion >= 12 * (LIM_F_PLANT * 1.25) - 1) {
					planta[x][y].estado = 0;
					planta[x][y].animacion = 2 * (LIM_F_PLANT * 2);
					planta[x][y].tiemp = 0;
					planta[x][y].esp.zombie_detect = false;
				}
				else {
					//Daño en área
					switch (planta[x][y].tiemp) {
					case 9:
						if (planta[x][y].esp.zombie_detect) {
							generar_proyectil(proyectil[y], y, planta[x][y].pos, x);
						}
						break;
					}
				}
			}
			break;
		case 14://SETA MIEDICA
			//Se esconde si detecta un zombi cerca
			if (planta[x][y].estado <= 1 && planta[x][y].esp.zombie_detect) {
				planta[x][y].estado = 2;
				planta[x][y].animacion = 0;
				planta[x][y].tiemp = 0;
			}
			switch (planta[x][y].estado) {
			case 0://Normal
				if (cant_zombi[y] - cant_zombi_ignorar[y] > 0 && planta[x][y].tiemp >= 12 + rand() % 4) {
					planta[x][y].tiemp = -1;
					planta[x][y].estado = 1;
					planta[x][y].animacion = 2 * LIM_F_PLANT - LIM_F_PLANT / 4 * 3;
				}
				break;
			case 1://Ataque
				if (planta[x][y].tiemp >= 3) {
					generar_proyectil(proyectil[y], y, planta[x][y].pos, x);
					planta[x][y].tiemp = -1;
					planta[x][y].animacion = 2 * LIM_F_PLANT;
					planta[x][y].estado = 0;
				}
				break;
			case 2://Animación esconderse
				if (planta[x][y].animacion >= 4 * LIM_F_PLANT - 1) {
					planta[x][y].estado = 3;
					planta[x][y].tiemp = 0;
					planta[x][y].animacion = 0;
				}
				break;
			case 3://Escondida
				if (!planta[x][y].esp.zombie_detect) {
					planta[x][y].estado = 4;
					planta[x][y].tiemp = 0;
					planta[x][y].animacion = 0;
				}
				if (planta[x][y].esp.zombie_detect && planta[x][y].tiemp > 5) {
					planta[x][y].esp.zombie_detect = false;
				}
				break;
			case 4://Animación esconderse
				if (planta[x][y].animacion >= 4 * (LIM_F_PLANT / 2) - 1) {
					planta[x][y].estado = 0;
					planta[x][y].tiemp = 0;
					planta[x][y].animacion = 0;
				}
				break;
			}
			break;
		case 15://Seta congelada
			if (planta[x][y].animacion >= 14 * (LIM_F_PLANT * 1.25) - 1) {
				Zombie* zomb_act{ NULL };
				vfx_pantalla(53, 153, 240, .1, 480);
				vfx_destello(255, 255, 255, 1, 100);
				for (int y{ 0 }; y < CAS_Y; y++) {
					zomb_act = zombie[y]->sig_zomb;
					while (zomb_act != NULL) {
						zomb_act->pv--;
						zomb_act->estado = 2;
						zomb_act->tiemp = 80;

						zomb_act = zomb_act->sig_zomb;
					}
				}
				eliminar_planta(planta[x][y]);
			}
			break;
		case 16://Petaseta
			switch(planta[x][y].estado) {
			case 0://se infla
				if (planta[x][y].animacion >= 5 * LIM_F_PLANT - 1) {
					planta[x][y].tiemp = 0;
					planta[x][y].animacion = 0;
					planta[x][y].estado = 1;
					vfx_destello(228, 203, 251, 0.95, 100);
					vfx_explosion_petaseta(x * 100 + 95, y * 100);
					for (int sx{ -3 }; sx <= 3; sx++) {
						if (x + sx >= 0 && x + sx < CAS_X_EXPL) {
							for (int sy{ -3 }; sy <= 3; sy++) {
								if (y + sy >= 0 && y + sy < CAS_Y) {
									explosion_tablero[x + sx][y + sy].danio += 500;
									explosion_tablero[x + sx][y + sy].tipo_area = 1;
								}
							}
						}
					}
				}
				break;
			case 1://Evita hacer daño doble
				for (int sx{ -3 }; sx <= 3; sx++) {
					if (x + sx >= 0 && x + sx < CAS_X_EXPL) {
						for (int sy{ -3 }; sy <= 3; sy++) {
							if (y + sy >= 0 && y + sy < CAS_Y) {
								explosion_tablero[x + sx][y + sy].danio -= 500;
								explosion_tablero[x + sx][y + sy].tipo_area = 0;
							}
						}
					}
				}
				eliminar_planta(planta[x][y]);
				generar_planta(-2, planta[x][y]);
				break;
			}
			break;
		case -2: //Cráter
			if (planta[x][y].tiemp >= 1500) {
				eliminar_planta(planta[x][y]);
			}
			break;
		}
		if (!pausa_total) {
			planta[x][y].tiemp++;
		}
	}
}

void generar_planta(short id_planta, Planta& planta) {
	planta.pv = PV_PLANTA[id_planta];
	planta.pos = id_planta;
	planta.tiemp = 0;
	planta.estado = 0;

	planta.animacion = 0;
	planta.anim_danio = 0;

	planta.dx = rand() % 13 - 6;
	planta.dy = rand() % 13 - 6;

	switch (id_planta) {
	case 2://Girasol
		planta.tiemp = 20 + rand() % 10;
		break;
	case 4: case 12://Nuez y Calabaza
		planta.esp.mordidas = 0;
		break;
	case 7://Carroñívora
		planta.esp.coord_zomb = 0;
		break;
	case 8://Repetidora
		planta.esp.repet = 0;
		break;
	case 10://Seta Solar
		planta.tiemp = 15 + rand() % 25;
		planta.esp.crecimiento = 11;
		break;
	case 11: case 9: case 14://Humoseta Honguito, seta miedica
		planta.esp.zombie_detect = false;
		break;
	case -2:
		planta.pv = 1;
		break;
	}
	std::cout << "PV[" << id_planta << "] = " << planta.pv << std::endl;
}

void plantar_planta() {
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
			short costo_planta;
			costo_planta = COST_PLANTA[planta_elegida];
			if ((tipo_patio + 1) % 2 && EXTR_SOL_DIA[planta_elegida] != 0) {
				costo_planta += EXTR_SOL_DIA[planta_elegida];
			}
			if (planta_elegida > 0 && !planta[pos_x][pos_y].pos && soles_guard >= costo_planta) {
				soles_guard -= costo_planta;
				soles_guard_suma -= costo_planta;
				semillero[semillero_elegido].recarga = REC_PLANTA[semillero[semillero_elegido].plant];
				if (semillero[semillero_elegido].aleat) {
					short planta_ant = semillero[semillero_elegido].plant;
					do {
						semillero[semillero_elegido].plant = rand() % CANT_PLANT + 1;
					} while (semillero[semillero_elegido].plant == planta_ant);
				}
				generar_planta(planta_elegida, planta[pos_x][pos_y]);
				semillero_elegido = -1;
				std::cout << "PLANTA " << planta_elegida << " PLANTADA EN \t[" << pos_x << "][" << pos_y << "]" << std::endl;
			}
		}
		//DESPLANTAR UNA PLANTA
		else if (planta_elegida == -1 && planta[pos_x][pos_y].pos > 0) {
			eliminar_planta(planta[pos_x][pos_y]);
			std::cout << "ASESINATO EN \t\t[" << pos_x << "][" << pos_y << "]" << std::endl;
		}
		semillero_elegido = -1;
		planta_elegida = 0;
	}
}

void seleccionar_planta(short pos_semillero) {
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
	if (pos_x != -1) {
		if (!planta_elegida) {
		ASIGNAR_PLANTA:
			if (pos_x != LIM_SEM) {
				short costo_planta;
				costo_planta = COST_PLANTA[semillero[pos_x].plant];
				if ((tipo_patio + 1) % 2 && EXTR_SOL_DIA[semillero[pos_x].plant] != 0) {
					costo_planta += EXTR_SOL_DIA[semillero[pos_x].plant];
				}
				if (soles_guard >= costo_planta && semillero[pos_x].recarga <= 0) {
					planta_elegida = semillero[pos_x].plant;
					semillero_elegido = pos_x;
				}
			}
			else {
				planta_elegida = -1;
				semillero_elegido = -1;
			}
		}
		else if (planta_elegida == semillero[pos_x].plant || (planta_elegida == -1 && pos_x == LIM_SEM)) {
			planta_elegida = 0;
			semillero_elegido = -1;
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
			if (!pausa_total) {
				if (planta[x][y].animacion < 8 * LIM_F_PLANT - 1) planta[x][y].animacion++;
				else if (planta[x][y].animacion >= 8 * LIM_F_PLANT - 1) planta[x][y].animacion = 0;
			}
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
			if (!pausa_total) {
				if (planta[x][y].animacion < 4 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			}
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
			if (!pausa_total) {
				if (planta[x][y].animacion < 15 * LIM_F_PLANT - 1) planta[x][y].animacion++;
				else if (planta[x][y].animacion >= 15 * LIM_F_PLANT - 1) planta[x][y].animacion = 0;
			}
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
			if (!pausa_total) {
				if (planta[x][y].animacion < 4 && !(frames % 7)) planta[x][y].animacion++;
				else if (planta[x][y].animacion == 4 && planta[x][y].tiemp >= 11) planta[x][y].animacion++;
			}
			return planta[x][y].animacion * 90 + 810;
		}
		break;

	case 3://PETACEREZA
		switch (planta[x][y].estado) {
		case 0:
			if (!pausa_total) {
				if (planta[x][y].animacion < 6 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			}
			return int(planta[x][y].animacion / LIM_F_PLANT) * 90;
		default:
			return -90;
		}
		break;

	case 4://NUEZ
		if (planta[x][y].pv > PV_PLANTA[4] * .6) {
			return 0 + (planta[x][y].tiemp % 20 ? 0 : 1) * 360;
		}
		else if (planta[x][y].pv > PV_PLANTA[4] * .4 && planta[x][y].pv <= PV_PLANTA[4] * .6) {
			return 90 + (planta[x][y].tiemp % 18 ? 0 : 1) * 360;
		}
		else if (planta[x][y].pv > PV_PLANTA[4] * .2 && planta[x][y].pv <= PV_PLANTA[4] * .4) {
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
			if (!pausa_total) {
				planta[x][y].animacion++;
			}
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
			if (!pausa_total) {
				if (planta[x][y].animacion < 4 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			}
			return int(planta[x][y].animacion / LIM_F_PLANT) * 90 + 1080;
		case 3: case 4:
			if (!pausa_total) {
				if (planta[x][y].animacion < 9 * LIM_F_EXPLO - 1) planta[x][y].animacion++;

			}
			return -90;
		}
		break;

	case 6://HIELAGUISANTES
		switch (planta[x][y].estado) {
		case 0://NORMAL
			if (!pausa_total) {
				if (planta[x][y].animacion < 8 * LIM_F_PLANT - 1) planta[x][y].animacion++;
				else if (planta[x][y].animacion >= 8 * LIM_F_PLANT - 1) planta[x][y].animacion = 0;
			}
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
			if (!pausa_total) {
				if (planta[x][y].animacion < 4 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			}
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
			if (!pausa_total) {
				if (planta[x][y].animacion < 8 * LIM_F_PLANT - 1) planta[x][y].animacion++;
				else if (planta[x][y].animacion >= 8 * LIM_F_PLANT - 1) planta[x][y].animacion = 0;

			}
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
				if (!pausa_total) {
					if (planta[x][y].animacion < 15 * LIM_F_PLANT - 1) planta[x][y].animacion++;
				}
				switch (planta[x][y].animacion / LIM_F_PLANT) {
				case 5: case 8: case 11:
					return 900;

				case 6: case 9: case 12:
					if (!(rand() % 32) && !pausa_total) {
						generar_particula(x * 100 + 168, y * 100 + 130, rand() % 3 + 5, rand() % 2, y);
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
			if (!pausa_total) {
				if (planta[x][y].animacion < 4 * LIM_F_PLANT - 1) planta[x][y].animacion++;
				else if (planta[x][y].animacion >= 4 * LIM_F_PLANT - 1) planta[x][y].animacion = 0;
			}
			return int(planta[x][y].animacion / LIM_F_PLANT) * 90 + 450;
		}
		break;

	case 8://REPETIDORA
		switch (planta[x][y].estado) {
		case 0://NORMAL
			if (!pausa_total) {
				if (planta[x][y].animacion < 8 * LIM_F_PLANT - 1) planta[x][y].animacion++;
				else if (planta[x][y].animacion >= 8 * LIM_F_PLANT - 1) planta[x][y].animacion = 0;

			}
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
			if (!pausa_total) {
				if (planta[x][y].animacion < 4 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			}
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0: return 0;
			case 1: return 450;
			case 2: return 540;
			default: return 630;
			}
			break;
		}
		break;

	case 9://Seta desesporada
		switch (planta[x][y].estado) {
		case 0:
			if (!pausa_total) {
				if (planta[x][y].animacion < 10 * LIM_F_PLANT - 1) planta[x][y].animacion++;
				else if (planta[x][y].animacion >= 10 * LIM_F_PLANT - 1) planta[x][y].animacion = 0;
			}
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0: case 2:
				return 0;
			case 1:
				return 90;
			case 3: case 9:
				return 180;
			case 4: case 8:
				return 270;
			case 5: case 7:
				return 360;
			case 6:
				return 450;
			}
			break;
		case 1:
			if (!pausa_total) {
				if (planta[x][y].animacion < 4 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			}
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0: return 0;
			case 1: return 540;
			case 2: return 630;
			default: return 720;
			}
			break;
		}
		break;

	case 10://Seta solar
		switch (planta[x][y].estado) {
		case 0://Chiquita
			if (!pausa_total) {
				if (planta[x][y].animacion < 8 * LIM_F_PLANT - 1) planta[x][y].animacion++;
				else if (planta[x][y].animacion >= 8 * LIM_F_PLANT - 1) planta[x][y].animacion = 0;
			}
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0:
				return 0;
			case 1: case 7:
				return 90;
			case 2: case 6:
				return 180;
			case 3: case 5:
				return 270;
			case 4:
				return 360;
			}
			break;
		case 1://Da sol chiquito
			if (!pausa_total) {
				if (planta[x][y].animacion < 2 && !(frames % 7)) planta[x][y].animacion++;
				else if (planta[x][y].animacion == 2 && planta[x][y].tiemp >= 9) planta[x][y].animacion++;
			}
			return planta[x][y].animacion * 90 + 450;
			break;
		case 2://Crece
			if (!pausa_total) {
				if (planta[x][y].animacion < 5 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			}
			return int(planta[x][y].animacion / LIM_F_PLANT) * 90 + 810;
			break;
		case 3://Grande
			if (!pausa_total) {
				if (planta[x][y].animacion < 10 * LIM_F_PLANT - 1) planta[x][y].animacion++;
				else if (planta[x][y].animacion >= 10 * LIM_F_PLANT - 1) planta[x][y].animacion = 0;
			}
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0:
				return 0;
			case 1: case 9:
				return 90;
			case 2: case 8:
				return 180;
			case 3: case 7:
				return 270;
			case 4: case 6:
				return 360;
			case 5:
				return 450;
			}
			break;
		case 4://Da sol grande
			if (!pausa_total) {
				if (planta[x][y].animacion < 3 && !(frames % 7)) planta[x][y].animacion++;
				else if (planta[x][y].animacion == 3 && planta[x][y].tiemp >= 9) planta[x][y].animacion++;
			}
			return planta[x][y].animacion * 90 + 540;
			break;
		}
		break;

	case 11://Humoseta
		switch (planta[x][y].estado) {
		case 0:
			if (!pausa_total) {
				if (planta[x][y].animacion < 10 * (LIM_F_PLANT * 2) - 1) planta[x][y].animacion++;
				else if (planta[x][y].animacion >= 10 * (LIM_F_PLANT * 2) - 1) planta[x][y].animacion = 0;
			}
			switch (int(planta[x][y].animacion / (LIM_F_PLANT * 2))) {
			case 0: case 4:
				return 0;
			case 1: case 3:
				return 90;
			case 2:
				return 180;
			case 5: case 9:
				return 540;
			case 6: case 8:
				return 630;
			case 7:
				return 720;
			}
			break;
		case 1:
			if (!pausa_total) {
				if (planta[x][y].animacion < 12 * (LIM_F_PLANT * 1.25) - 1) planta[x][y].animacion++;
			}
			if (int(planta[x][y].animacion / (LIM_F_PLANT * 1.25)) <= 9) {
				return (int(planta[x][y].animacion / (LIM_F_PLANT * 1.25))) * 90 + 540;
			}
			else {
				return (15 - int(planta[x][y].animacion / (LIM_F_PLANT * 1.25))) * 90;
			}
			break;
		}
		break;

	case 12://Calabaza
		if (!corte_vert) {	//Calabaza normal
			if (planta[x][y].pv > PV_PLANTA[12] * .6) {
				return 0;
			}
			else if (planta[x][y].pv > PV_PLANTA[12] * .4 && planta[x][y].pv <= PV_PLANTA[12] * .6) {
				return 180;
			}
			else if (planta[x][y].pv > PV_PLANTA[12] * .2 && planta[x][y].pv <= PV_PLANTA[12] * .4) {
				return 360;
			}
			else {
				return 540;
			}
		}
		else {				//Fondo calabaza
			if (planta[x][y].pv > PV_PLANTA[12] * .6) {
				return 90;
			}
			else if (planta[x][y].pv > PV_PLANTA[12] * .4 && planta[x][y].pv <= PV_PLANTA[12] * .6) {
				return 270;
			}
			else if (planta[x][y].pv > PV_PLANTA[12] * .2 && planta[x][y].pv <= PV_PLANTA[12] * .4) {
				return 450;
			}
			else {
				return 630;
			}
		}
		break;

	case 13://Hipnoseta
		switch (planta[x][y].estado) {
		default:
			if (!pausa_total) {
				if (planta[x][y].animacion < 12 * (LIM_F_PLANT * 1.25) -1) planta[x][y].animacion++;
				else if (planta[x][y].animacion >= 12 * (LIM_F_PLANT * 1.25) - 1) planta[x][y].animacion = 0;
			}
			switch (int(planta[x][y].animacion / (LIM_F_PLANT * 1.25))) {
			case 0: case 11:
				return 90;
			case 10:
				return 180;
			case 1: case 9:
				return 270;
			case 2: case 8:
				return 360;
			case 3: case 7:
				return 450;
			case 4:
				return 540;
			case 5: case 6:
				return 630;
			}
		}
		break;

	case 14://Seta miédica
		switch (planta[x][y].estado) {
		case 0://no hace nada
			if (!pausa_total) {
				if (planta[x][y].animacion < 8 * LIM_F_PLANT - 1) planta[x][y].animacion++;
				else if (planta[x][y].animacion >= 8 * LIM_F_PLANT - 1) planta[x][y].animacion = 0;
			}
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0:
				return 0;
			case 1: case 7:
				return 90;
			case 2: case 6:
				return 180;
			case 3: case 5:
				return 270;
			case 4:
				return 360;
			}
			break;
		case 1://Dispara
			if (!pausa_total) {
				if (planta[x][y].animacion < 4 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			}
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0: return 0;
			case 1: return 450;
			case 2: return 540;
			default: return 630;
			}
			break;
		case 2://Ve un zombi
			if (!pausa_total) {
				if (planta[x][y].animacion < 4 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			}
			switch (int(planta[x][y].animacion / LIM_F_PLANT)) {
			case 0: return 720;
			case 1: return 810;
			case 2: return 900;
			default: return 990;
			}
			break;
		case 3://Se esconde
			if (!pausa_total) {
				if (!(frames % 7)) {
					short frame_ant = planta[x][y].animacion;
					do {
						planta[x][y].animacion = rand() % 4;
					} while (frame_ant == planta[x][y].animacion);
				}
			}
			switch (planta[x][y].animacion) {
			case 0: return 1080;
			case 1: return 1170;
			case 2: return 1260;
			case 3: return 1350;
			}
			break;
		case 4://Sale
			if (!pausa_total) {
				if (planta[x][y].animacion < 4 * (LIM_F_PLANT / 2) -1) planta[x][y].animacion++;
			}
			switch (int(planta[x][y].animacion / (LIM_F_PLANT / 2))) {
			default: return 720;
			case 2: return 810;
			case 1: return 900;
			case 0: return 990;
			}
			break;
		}
		break;

	case 15://Seta congelada
		switch (planta[x][y].estado) {
		case 0://explota
			if (!pausa_total) {
				if (planta[x][y].animacion < 14 * (LIM_F_PLANT * 1.25) - 1) planta[x][y].animacion++;
			}
			switch (int(planta[x][y].animacion / (LIM_F_PLANT * 1.25))) {
			case 0: case 4: case 8:
				return 0;
			case 1: case 3: case 5: case 7: case 9:
				return 90;
			case 2: case 6: case 10:
				return 180;
			case 11:
				return 270;
			case 12:
				return 360;
			case 13:
				return 450;
			}
			break;
		}
		break;

	case 16://Petaseta
		switch (planta[x][y].estado) {
		case 0:
			if (!pausa_total) {
				if (planta[x][y].animacion < 5 * LIM_F_PLANT - 1) planta[x][y].animacion++;
			}
			return int(planta[x][y].animacion / LIM_F_PLANT) * 90;
		default:
			return -90;
		}
		break;

	case -2://cráter
		if (planta[x][y].tiemp < 800) {
			return 450;
		}
		else if (planta[x][y].tiemp < 1200) {
			return 540;
		}
		else {
			return 630;
		}
		break;
	}
	//Caso base
	return 0;
}

/*----------FUNCIONES-SOL-------------------------------------------------------------------------------------------------*/

bool funcion_sol(Sol& sol, short pos) {
	short x, y;
	float rango;
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
		if (mouse.estado == 2) {
			rango = 420;
		}
		else {
			rango = 180;
		}
		if (mouse.x > x - rango * sol.tam && mouse.x < x + rango * sol.tam && mouse.y > y - rango * sol.tam && mouse.y < y + rango * sol.tam) {
			std::cout << "SOL RECOLECTADO" << std::endl;
			cant_sol_recol += sol.cant;
			soles_guard += sol.cant;
			soles_guard_suma += sol.cant;
			oleada.puntos += 10 * float(sol.cant) / 25;
			oleada.puntos_sum += 10 * float(sol.cant) / 25;
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
				sol.ant_sol->sig_sol = siguiente_sol;

				anterior_sol = sol.ant_sol;
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
	if (!pausa_total) {
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
	nuevo_sol->tam = float(nuevo_sol->cant) / 25 * .6 + .4;
	if (pos_y) {
		nuevo_sol->estado_act = 0;
		nuevo_sol->x = pos_x + 22;
		nuevo_sol->y = pos_y + 60 - rand() % 12 * 6;
		nuevo_sol->estado.anim.impulso = (rand() % 25 + 1) + 20;
		nuevo_sol->estado.anim.d = rand() % 2;
		nuevo_sol->estado.anim.mov_x = 0;
	}
	else {
		nuevo_sol->estado_act = -1;
		nuevo_sol->x = pos_x;
		nuevo_sol->y = -100;
		nuevo_sol->estado.cayendo.mov_y = rand() % (RESOL_Y - 400) + 200;
	}
	cant_sol_tablero++;
}

/*----------FUNCIONES-PROYECTIL-------------------------------------------------------------------------------------------*/

bool funcion_proyectil(Proyectil &proy, short fila) {
	Zombie *zomb_atacado = NULL, *zomb_busqueda = NULL;
	zomb_busqueda = zombie[fila];
	//Busca un zombie a atacar
	if (proy.tipo >= 0) {
		if (proy.tipo == 4) {
			short x = proy.cas_x_ini;
			switch(proy.tiempo_ev){
			case 0:
				explosion_tablero[x][fila].danio += 1;
				break;
			case 1:
				explosion_tablero[x + 1][fila].danio += 1;
				explosion_tablero[x][fila].danio -= 1;
				break;
			case 2:
				explosion_tablero[x + 2][fila].danio += 1;
				explosion_tablero[x + 1][fila].danio -= 1;
				break;
			case 3:
				explosion_tablero[x + 2][fila].danio -= 1;
				break;
			case 4:
				if (x + 3 < CAS_X_EXPL) {
					explosion_tablero[x + 3][fila].danio += 1;
				}
				break;
			case 5:
				if (x + 3 < CAS_X_EXPL) {
					explosion_tablero[x + 3][fila].danio -= 1;
				}
				break;
			case 6:
				if (x + 4 < CAS_X_EXPL) {
					explosion_tablero[x + 4][fila].danio += 1;
				}
				break;
			case 7:
				if (x + 4 < CAS_X_EXPL) {
					explosion_tablero[x + 4][fila].danio -= 1;
				}
				break;
			}
			if (proy.esp.trasl_x >= 0) {
				goto Eliminar_proyectil;
			}
			else {
				return false;
			}
		}
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
			case 1://Guisante congelado
				zomb_atacado->pv -= 1;
				if (zomb_atacado->estado != 2) {	//Evita cambiar el estado del zombi congelado 
					zomb_atacado->estado = 1;
					zomb_atacado->tiemp = 60;
				}
				proy.tipo = -2;
				break;
			case 2://Espora desesporada
				zomb_atacado->pv -= 1;
				proy.tipo = -3;
				break;
			case 3://Espora Miédica
				zomb_atacado->pv -= 1;
				proy.tipo = -4;
				break;
			}
			if (zomb_atacado->anim_danio < 18) {
				zomb_atacado->anim_danio = 24;
			}
			proy.tiempo_ev = 0;
		}
	}
	else {
		if (proy.tiempo_ev >= 3) {
			//borra el proyectil
			Eliminar_proyectil:
			if (proy.sig_proy) {
				proy.ant_proy->sig_proy = proy.sig_proy;
				proy.sig_proy->ant_proy = proy.ant_proy;
			}
			else {
				proy.ant_proy->sig_proy = NULL;
			}
			cant_proy[fila]--;
			delete& proy;
			return true;
		}
	}
	return false;
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

	//Asignar campos
	nuevo_proy->y = 0;
	nuevo_proy->cas_x_ini = pos_x;
	switch (tipo) {
	case 1: case 8://Lanzaguisantes y repetidora
		nuevo_proy->y = y * 100 + 189;
		nuevo_proy->x = pos_x * 100 + 270;
		nuevo_proy->tipo = 0;
		break;
	case 6://Hielaguisantes
		nuevo_proy->y = y * 100 + 189;
		nuevo_proy->x = pos_x * 100 + 270;
		nuevo_proy->tipo = 1;
		break;
	case 9://Honguito Desesporado
		nuevo_proy->y = y * 100 + 218;
		nuevo_proy->x = pos_x * 100 + 250;
		nuevo_proy->tipo = 2;
		break;
	case 14://Honguito Chillón
		nuevo_proy->y = y * 100 + 200;
		nuevo_proy->x = pos_x * 100 + 270;
		nuevo_proy->tipo = 3;
		break;
	case 11://Humoseta
		nuevo_proy->y = y * 100 + 100;
		nuevo_proy->esp.trasl_x = -360;
		nuevo_proy->x = pos_x * 100 + 195 - nuevo_proy->esp.trasl_x;
		nuevo_proy->tipo = 4;
		break;
	}
	nuevo_proy->y += planta[pos_x][y].dy;
	nuevo_proy->tiempo_ev = 0;
	cant_proy[y]++;
}

bool mover_proyectil(Proyectil& proy, short pos_y) {
	if (pausa_total) {
		return false;
	}
	if (proy.tipo < 0) {
		proy.x += 1;
		return false;
	}
	if (proy.tipo == 4) {
		proy.esp.trasl_x /= 1.03125;
		if ((int)proy.esp.trasl_x >= -1) {
			proy.esp.trasl_x = 0;
		}
		return false;
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
		return true;
	}
	proy.x += 5;
	return false;
}

/*----------FUNCIONES-ZOMBIE----------------------------------------------------------------------------------------------*/

bool funcion_zombie(Zombie& zomb, short fila) {
	//Rebajar tiempo estado
	if (zomb.estado != 0 && zomb.tiemp > 0) {
		zomb.tiemp--;
	}
	else {
		switch (zomb.estado) {
		case 1://Enfriado
			zomb.estado = 0;
			break;
		case 2://Congelado
			zomb.estado = 1;
			zomb.tiemp = 100;
			break;
		}
	}
	//revisar estado del zombi
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
				generar_particula(zomb.x, fila * 100 + 120, 1, 1, fila);
			}
			break;
		case 2://Cubeta
			if (zomb.pv <= 23 && zomb.est_danio == 2) {
				zomb.est_danio = 3;
			}
			else if (zomb.pv <= 10 && zomb.est_danio == 3) {
				zomb.est_danio = 0;
				generar_particula(zomb.x, fila * 100 + 120, 2, 1, fila);
			}
			break;
		case 3://Ladrillo
			if (zomb.pv <= 96 && zomb.est_danio == 4) {
				zomb.est_danio = 5;
				for (int i{ rand() % 2 + 1 }; i > 0; i--) {
					generar_particula(zomb.x, fila * 100 + 120, 8, 1, fila);
				}
			}
			if (zomb.pv <= 40 && zomb.est_danio == 5) {
				zomb.est_danio = 2;
				for (int i{ rand() % 3 + 1 }; i > 0; i--) {
					generar_particula(zomb.x, fila * 100 + 120, 8, 1, fila);
				}
			}
			if (zomb.pv <= 18 && zomb.est_danio == 2) {
				zomb.est_danio = 3;
				for (int i{ rand() % 4 + 1 }; i > 0; i--) {
					generar_particula(zomb.x, fila * 100 + 120, 8, 1, fila);
				}
			}
			if (zomb.pv <= 10 && zomb.est_danio == 3) {
				zomb.est_danio = 0;
				for (int i{ rand() % 2 + 1 }; i > 0; i--) {
					generar_particula(zomb.x, fila * 100 + 120, 8, 1, fila);
				}
			}
			break;
		}
		if (zomb.pv < 5 && zomb.est_danio == 0) {
			zomb.est_danio = 1;
			generar_particula(zomb.x, fila * 100 + 120, 5, 1, fila);
		}

		//Encontrar Casilla
		for (int i{}; i < CAS_X_EXPL; i++) {
			if (zomb.x >= 150 + i * 100 && zomb.x < 250 + i * 100) {
				casilla = i;
			}
		}

		//Busca en las casillas del juego
		if (casilla >= 0) {
			bool danio_recibido{ false };

			//Si se encuentra en el borde de la casilla, se revisa una casilla a la derecha
			if ((int(zomb.x) - 150) % 100 >= 90) {
				sx = 1;
			}
			else {
				sx = 0;
			}

			//Busca por áreas de daño
			for (int x{ sx + casilla }; x >= casilla && !danio_recibido; x--) {
				if (explosion_tablero[x][fila].danio > 0 && x < CAS_X_EXPL) {
					if (sx == 0) {	//Si sx es 0, significa que está en su casilla original
						zomb.pv -= explosion_tablero[x][fila].danio;
					}//Si sx es distinto de 0, revisa en otras casillas, por lo que se hacen varias confirmaciones
					// La primera verificación es específica para la explosión.
					// La segunda verificación es para evitar que no reciba daño de la humoseta (puede que reciba el doble)
					else if (explosion_tablero[x][fila].tipo_area == 1 || (int(zomb.x) - 150) % 100 >= 98) {
						zomb.pv -= explosion_tablero[x][fila].danio;
						danio_recibido = true;
					}
					//Si el zombi murió por la explosión
					if (zomb.pv <= 0 && explosion_tablero[x][fila].tipo_area == 1) {
						zomb.comiendo = false;
						zomb.estado = 0;
						zomb.tiemp = 0;
						zomb.est_danio = -4;
						zomb.animacion = 0;
						zomb.tiemp_muert = 0;
						return false;
					}
					//Si sobrevivió, se le aplica la animación de daño
					else if (zomb.anim_danio < 18) {
						zomb.anim_danio = 24;
					}
				}
			}

			//Revisa las plantas y eventos en las casillas
			if (casilla < CAS_X) {
				//busca si se encuentra con una planta
				if (planta[casilla][fila].pos) {
					if (!(zomb.estado == 1 && frames % 20) && zomb.estado != 2) {
						switch (planta[casilla][fila].pos) {
						case 4: case 12://Nuez y calabaza
							if (planta[casilla][fila].esp.mordidas < 3) {//es una nuez
								planta[casilla][fila].pv--;
								planta[casilla][fila].esp.mordidas++;
								if (!planta[casilla][fila].anim_danio) { //animación de daño de la planta
									planta[casilla][fila].anim_danio = 24;
								}
							}
							zomb.comiendo = true;
							break;
						case -2: case 3: case 15: case 16://Petacereza, Seta congelada y Petaseta
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
					zomb.pv -= 100;
					if (zomb.pv <= 0) {
						if (zomb.est_danio >= 2) {
							short cant_part{ 0 }, cant_part_obl{ 1 };
							switch (zomb.id) { //Suelta el gorrito
							case 1: generar_particula(zomb.x, fila * 100 + 120, zomb.est_danio == 2 ? 3 : 1, 1, fila); break;
							case 2: generar_particula(zomb.x, fila * 100 + 120, zomb.est_danio == 2 ? 4 : 2, 1, fila); break;
							case 3://Ladrillo
								switch (zomb.est_danio) {
								case 4: cant_part = 10;cant_part_obl = 3;	break;
								case 5: cant_part = 8; cant_part_obl = 3;	break;
								case 2: cant_part = 6; cant_part_obl = 2;	break;
								case 3: cant_part = 3;						break;
								}
								for (int i{ rand() % cant_part + cant_part_obl }; i > 0; i--) {
									generar_particula(zomb.x, fila * 100 + 120, 8, 1, fila);
								}
								break;
							}
						}
						//Genera una partícula brazo, pierna o corbata
						generar_particula(zomb.x, fila * 100 + 120, rand() % 3 + 5, 1, fila);
						goto eliminar_zombie;
					}
				}
			}

			//Busca plantas en un rango
			for (int y{ fila - 1 }; y <= fila + 1; y++) {
				if (y >= 0 && y < CAS_Y) {
					for (int x{ casilla - 4 }; x <= casilla  + 1; x++) {
						if (x >= 0 && x < CAS_X) {
							//Revisa enfrente
							if (x <= casilla && y == fila) {
								//Busca la humoseta
								if (planta[x][fila].pos == 11 && !planta[x][fila].esp.zombie_detect) {
									planta[x][fila].esp.zombie_detect = true;
								}
								if (x >= casilla - 3) {
									//Busca la seta desesporada
									if (planta[x][fila].pos == 9 && !planta[x][fila].esp.zombie_detect) {
										planta[x][fila].esp.zombie_detect = true;
									}
								}
							}
							if (x >= casilla - 1 && x <= casilla + 1) {
								if (planta[x][y].pos == 14) {
									planta[x][y].esp.zombie_detect = true;
									planta[x][y].tiemp = 0;
								}
							}
						}
					}
				}
			}

			//Busca una carroñívora
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
								short cant_part{ 0 }, cant_part_obl{ 1 };
								switch (zomb.id) {
								case 1: generar_particula(zomb.x, fila * 100 + 120, zomb.est_danio == 2 ? 3 : 1, 1, fila); break;
								case 2: generar_particula(zomb.x, fila * 100 + 120, zomb.est_danio == 2 ? 4 : 2, 1, fila); break;
								case 3://Ladrillo
									switch (zomb.est_danio) {
									case 4: cant_part = 10; cant_part_obl = 3;	break;
									case 5: cant_part = 8; cant_part_obl = 3;	break;
									case 2: cant_part = 6; cant_part_obl = 2;	break;
									case 3: cant_part = 3;						break;
									}
									for (int i{ rand() % cant_part + cant_part_obl }; i > 0; i--) {
										generar_particula(zomb.x, fila * 100 + 120, 8, 1, fila);
									}
									break;
								}
							}
							generar_particula(zomb.x, fila * 100 + 120, 0, 1, fila);
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
		if (zomb.estado != 2) {
			zomb.tiemp_muert++;
		}
		if (zomb.est_danio >= 0) {
			generar_particula(zomb.x, fila * 100 + 120, 0, 1, fila);
			zomb.tiemp_muert = 0;
			zomb.est_danio = -1;
		}
		//Se mueve antes de morir
		if (zomb.tiemp_muert >= 12 && zomb.est_danio == -1) {
			zomb.comiendo = false;
			zomb.tiemp_muert = 0;
			zomb.animacion = 0;
			zomb.est_danio = -2;
		}
		//Fallece
		if (zomb.animacion >= 6 * (LIM_F_ZOMB * .3) - 1 && zomb.est_danio == -2) {
			zomb.est_danio = -3;
			zomb.tiemp_muert = 0;
		}
		//Se termina de quemar
		if (zomb.animacion >= 14 * LIM_F_EXPLO - 1 && zomb.est_danio == -4) {
			zomb.est_danio = -5;
			zomb.tiemp_muert = 0;
		}
		if (zomb.tiemp_muert > 38) {
		eliminar_zombie:
			switch (zomb.id) {
			case 0:	oleada.puntos += 100;	oleada.puntos_sum += 100;	break;
			case 1:	oleada.puntos += 200;	oleada.puntos_sum += 200;	break;
			case 2:	oleada.puntos += 500;	oleada.puntos_sum += 500;	break;
			case 3:	oleada.puntos += 1200;	oleada.puntos_sum += 1200;	break;
			}
			cant_zombi_elim++;
			if (zomb.sig_zomb) {
				zomb.ant_zomb->sig_zomb = zomb.sig_zomb;
				zomb.sig_zomb->ant_zomb = zomb.ant_zomb;
			}
			else {
				zomb.ant_zomb->sig_zomb = NULL;
			}
			cant_zombi[fila]--;
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
			//Fin del juego
			if (zomb.x < 130 && !fin_juego) {
				fin_juego = true;
				pausa_total = true;
				tr.id = 4;
				tr.finalizado = false;
				tr.muerte.fila = fila;
				tr.muerte.tam = 10;
				tr.muerte.tiemp = 0;
				tr.muerte.x = zomb.x - 190;
				tr.muerte.y = fila * 100 + 120 - 630;
				tr.muerte.tam_letr = 0;
				tr.muerte.temblor = 5;
			}
			else {
				switch (zomb.estado) {
				case 1://Enfriado
					zomb.x -= .11;
					break;
				case 2://Congelado
					break;
				default://Normal
					zomb.x -= .22;
					break;
				}
			}
			break;
		case -2://Falleciendo
			if (zomb.estado != 2) {
				zomb.x -= 0.06;
			}
			break;//Muerto
		case -3: case -4: case -5:
			break;
		}
	}
}

void generar_zombie(short y, short tipo) {
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
	case 3://Zombie cara ladrillo
		nuevo_zomb->pv = 165;
		nuevo_zomb->est_danio = 4;
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
	short zombie_dif{ 0 }, random_zomb{ 0 }, total_zomb{ 0 };
	short generacion{ 0 };
	for (int i{}; i < CAS_Y; i++) {
		total_zomb += cant_zombi[i];
	}

	//Generar zombies si no hay zombies
	if (total_zomb <= 0 && frames > FRAMES_INI && oleada.zombie_ataq <= 0) {
		oleada.tiemp_no_zomb++;
		if (oleada.tiemp_no_zomb >= 120) {
			oleada.puntos += 1000;
			oleada.tiemp_no_zomb = 0;
			goto generar_nueva_oleada;
		}
	}
	else if (oleada.tiemp_no_zomb > 0) {
		oleada.tiemp_no_zomb = 0;
	}

	//Generar nueva oleada
	if (oleada.tiempo <= 0 && frames >= FRAMES_INI) {
		generar_nueva_oleada:
		oleada.zombie_ataq += oleada.dificultad;//Bajar el tiempo para la siguiente oleada
		if (oleada.tiempo_oleada > oleada.lim_tiempo_oleada && frames > FRAMES_INI) {
			oleada.tiempo_oleada -= (oleada.tiempo_oleada * .04) + (oleada.puntos + frames) * .00000001 - 1;//MODIFICADO 0.8 org
		}//Si es menor a limiteTiempoOleada, se hace limiteTiempoOleada
		if (oleada.tiempo_oleada < oleada.lim_tiempo_oleada) {
			oleada.tiempo_oleada = oleada.lim_tiempo_oleada;
		}
		oleada.tiempo = oleada.tiempo_oleada;
		std::cout << "OLEADA: " << oleada.zombie_ataq << std::endl;
	}
	else {
		oleada.tiempo--;
	}

	//GENERAR ZOMBIES EN EL TABLERO
	if (oleada.zombie_ataq > 0) {
		generacion = (20 * float(oleada.dificultad) / (oleada.zombie_ataq + 1));
		if (generacion > 20) {
			generacion = 20;
		}
		else if (generacion <= 0) {
			generacion = 1;
		}
		if (frames % generacion == 0 && !(rand() % 4)) {
			bool zomb_aparecido[CAS_Y], lleno{ false };
			float mult_dificult;

			//Determinar filas en las que pueden aparecer zombis
			for (int i{}; i < CAS_Y; i++) {
				if (cant_zombi[i] > total_zomb / CAS_Y) {
					zomb_aparecido[i] = true;
				}
				else {
					zomb_aparecido[i] = false;
				}
			}

			//Spawnear zombis
			while (oleada.zombie_ataq > 0 && !lleno) {
				short fila_aparicion, cant_aparicion{ 0 };
				random_zomb = rand() % oleada.zombie_ataq + 1;		//Elegir zombi aleatorio

				//Revisar si no se spawnearon la cantidad necesaria de zombis
				for (int i{}; i < CAS_Y; i++) {
					if (zomb_aparecido[i]) {
						cant_aparicion++;
					}
					if (cant_aparicion >= CAS_Y) {
						lleno = true;
						break;
					}
				}
				if (lleno) {
					break;
				}

				//Spawnear en una posición válida
				do {
					fila_aparicion = rand() % CAS_Y;
				} while (zomb_aparecido[fila_aparicion]);

				//Multiplicador de dificultad
				mult_dificult = float(oleada.tiempo_oleada) / oleada.lim_tiempo_oleada;
				if (mult_dificult > 2) {
					mult_dificult = 2;
				}
				mult_dificult -= -1 * frames < 36000 ? float(frames) / 36000 : 1;

				//Seleccionar zombi a aparecer
				if (random_zomb < 3) {
					zombie_dif = 0;
					oleada.zombie_ataq -= 1;
				}
				else if (random_zomb >= 3 && random_zomb < 6) {
					zombie_dif = 1;
					oleada.zombie_ataq -= 3;
				}
				else if (random_zomb >= 6 && random_zomb < 13) {
					zombie_dif = 2;
					oleada.zombie_ataq -= 6;
					oleada.tiempo += oleada.tiempo_oleada * .05 * mult_dificult;
				}
				else if (random_zomb >= 20) {
					zombie_dif = 3;
					oleada.zombie_ataq -= 30;
					oleada.tiempo += oleada.tiempo_oleada * .1 * mult_dificult;
				}
				zomb_aparecido[fila_aparicion] = true;
				generar_zombie(fila_aparicion, zombie_dif);
				if (!(rand() % ((CAS_Y - cant_aparicion) * 2))) {
					break;
				}
			}
		}
	}
	if (frames > FRAMES_INI && (frames - FRAMES_INI) % oleada.ritmo_nivel == 0 && oleada.dificultad < oleada.lim_dificultad) {
		oleada.dificultad++;
		std::cout << "DIFICULTAD: " << oleada.dificultad << std::endl;
		if (oleada.ritmo_nivel > FRAMES_INI/2) {
			oleada.ritmo_nivel *= 0.85;
		}
		if (oleada.ritmo_nivel < FRAMES_INI/2) {
			oleada.ritmo_nivel = FRAMES_INI/2;
		}
	}
}

short animacion_zombie(Zombie& zomb) {
	switch (zomb.id) {
	case 0: case 1: case 2: case 3:
		if (zomb.comiendo) {
			if (!pausa_total && zomb.estado != 2) {
				if (zomb.animacion < 8 * LIM_F_ZOMB_COM - 1) zomb.animacion++;
				else if (zomb.animacion >= 8 * LIM_F_ZOMB_COM - 1) zomb.animacion = 0;

			}
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
			if (!pausa_total && zomb.estado != 2) {
				if (zomb.estado == 1 && frames % 2) {
					zomb.animacion--;
				}
				if (zomb.animacion < 12 * LIM_F_ZOMB - 1) zomb.animacion++;
				else if (zomb.animacion >= 12 * LIM_F_ZOMB - 1) zomb.animacion = 0;
			}
			return int(zomb.animacion / LIM_F_ZOMB) * 108;

		case -2: case -3://Animación muerte "natural"
			if (!pausa_total && zomb.estado != 2) {
				if (zomb.estado == 1 && frames % 2) {
					zomb.animacion--;
				}
				if (zomb.animacion < 6 * (LIM_F_ZOMB * .3) -1) zomb.animacion++;
			}
			return int(zomb.animacion / (LIM_F_ZOMB * .3)) * 108;

		case -4: case -5://Animación muerte quemado
			if (!pausa_total && zomb.estado != 2) {
				if (zomb.animacion < 20 * LIM_F_EXPLO - 1) zomb.animacion++;
				//Probabilidad de que parpadee de nuevo
				if (zomb.animacion == 10 * LIM_F_EXPLO && rand() % 2){
					zomb.animacion -= 4 * LIM_F_EXPLO;
				}
			}
			//Se queda atontado
			if (zomb.animacion < 6 * LIM_F_EXPLO) {
				return 0;
			}
			switch (int(zomb.animacion / LIM_F_EXPLO) - 6) {
			//Parpadea
			case 0: case 1:
				zomb.tiemp_muert = 0;
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
	if (!pausa_total) {
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
			cant_particulas[part.fila]--;
			delete& part;
			return true;
		}
	}
	return false;
}

void generar_particula(short x, short y, short id, bool dir, short fila) {
	Particula* nueva_part = NULL, * ant_part = NULL;
	nueva_part = ant_part = particulas[fila];
	for (int i{}; i < cant_particulas[fila]; i++) {
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
	nueva_part->fila = fila;
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
	case 8://Ladrillo
		nueva_part->estado = 0;
		nueva_part->impulso = rand() % 20 + 20;
		nueva_part->mov_x = nueva_part->impulso / 3;
		nueva_part->x = x + 44 - nueva_part->mov_x;
		nueva_part->angulo = rand() % 360;
		nueva_part->tiemp = 0;
		nueva_part->y = y - 18 + nueva_part->impulso;
		break;
	}
	nueva_part->dir = dir;
	cant_particulas[fila]++;
}

void rotacion_particula_x(Particula& part) {
	short dir = part.dir ? 1 : -1;
	if (pausa_total) {
		return;
	}
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
	case 5: case 6: case 7: case 8:
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
	if (!part.estado && !pausa_total) {
		//La partícula es una cabeza
		switch (part.id) {
		default:
			part.angulo += (part.impulso * dir - 18) / 3;
			break;
		case 5: case 6: case 8:
			part.angulo += (part.impulso * dir - 8) / 5;
			break;
		}
	}
	//Derecha
	if (!pausa_total) {
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
	}
	//Caso especial para la partícula brazo y pierna
	if (part.id == 5 || part.id == 6) {
		mult = .75;
		caida = 1;
	}
	//Retornar extra en y
	return part.y - sqrt(pow(float(part.impulso), 2) - pow(part.mov_x + part.impulso * (-dir), 2)) * mult + part.mov_x * caida * dir;
}

/*----------FUNCIONES-VFX-------------------------------------------------------------------------------------------------*/

bool funcion_efecto(Efectos_esp& eff) {
	ALLEGRO_COLOR color;
	float transp;
	color = NORM_C;
	bool eliminar{ false };
	switch (eff.tipo) {
	case 1: case 2://Destello y pantalla
		if (eff.pant.tiemp >= eff.pant.tiemp_max) {
			eliminar = true;
			break;
		}
		if (eff.tipo == 1) {	//Transparencia Destello
			transp = float(eff.pant.tiemp_max - eff.pant.tiemp) / eff.pant.tiemp_max * eff.pant.transp;
		}
		else {					//Transparencia Pantalla
			if (eff.pant.tiemp >= eff.pant.tiemp_max * .9) {
				transp = float(eff.pant.tiemp_max - eff.pant.tiemp) / eff.pant.tiemp_max * 10 * eff.pant.transp;
			}
			else {
				transp = eff.pant.transp;
			}
		}
		color = al_map_rgba_f(eff.pant.r * transp / 255, eff.pant.g * transp / 255, eff.pant.b * transp / 255, transp);
		al_draw_tinted_bitmap((ALLEGRO_BITMAP*)b.pantalla_blanca, color, 0, 0, 0);
		if (!pausa_total) {
			eff.pant.tiemp++;
		}
		break;
	case 3: //Explosión petacereza
		if (eff.expl.tiemp > 11) {
			eliminar = true;
			break;
		}
		if (!eliminar) {
			short temblor_y = pausa_total ? 2 : rand() % 6;
			short temblor_x = pausa_total ? 2 : rand() % 6;
			short trans = eff.expl.tiemp > 10 ? 255 - (eff.expl.tiemp - 10) * 100 : 255;
			al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.explosion, eff.expl.tiemp * 300, 0, 300, 300, NORM_C, 150, 150, eff.expl.x + 150, eff.expl.y + 150, 1, 1, float(eff.expl.aleatorio) * 0.0175, eff.expl.aleatorio % 2);
			al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.explosion, M_TRANS_C(trans), 2100, 300, 300, 200, eff.expl.x - 2 + temblor_x, eff.expl.y + 48 + temblor_y, 0);
			if (!pausa_total) {
				if (!(frames % (LIM_F_EXPLO))) eff.expl.tiemp++;
			}
		}
		break;
	case 4: //Explosión petaseta
		if (eff.expl.tiemp > 29) {
			eliminar = true;
			break;
		}
		if (!eliminar) {
			float tamanio = 1;
			short temblor_y = pausa_total ? 2 : rand() % 6;
			short temblor_x = pausa_total ? 2 : rand() % 6;
			short anim = eff.expl.tiemp > 3 ? int((eff.expl.tiemp - 3) / 2) : -1;
			short trans = eff.expl.tiemp > 25 ? 255 - (eff.expl.tiemp - 25) * 60 : 255;
			if (anim > 11) {
				anim = 11;
			}
			else if (anim < 0) {
				tamanio = .4 + eff.expl.tiemp * .2;
				anim = 0;
			}
			anim *= 300;
			al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.explosion, anim, 700, 300, 300, M_TRANS_C(trans), 0, 0, eff.expl.x + 150.0 * (1.0 - tamanio), eff.expl.y + 270.0 * (1.0 - tamanio), tamanio, tamanio, 0, eff.expl.aleatorio);
			al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.explosion, 2400, 300, 300, 200, M_TRANS_C(trans), 0, 0, eff.expl.x - 2 + temblor_x + 150.0 * (1.0 - tamanio), eff.expl.y + 48 + temblor_y + 320.0 * (1.0 - tamanio), tamanio, tamanio, 0, 0);
			if (!pausa_total) {
				if (!(frames % (LIM_F_EXPLO))) eff.expl.tiemp++;
			}
		}
		break;
	}
	if (eliminar) {
		eff.ant->sig = eff.sig;
		if (eff.sig) {
			eff.sig->ant = eff.ant;
		}
		std::cout << "EFFECTO ELIMINADO " << &eff << std::endl;
		delete& eff;
		return true;
	}
	return false;
}

void vfx_pantalla(short r, short g, short b, float transp, short tiemp_max) {
	Efectos_esp* eff{ NULL };
	eff = generar_efecto_fin();
	eff->tipo = 2;

	eff->pant.r = r;
	eff->pant.g = g;
	eff->pant.b = b;
	eff->pant.tiemp_max = tiemp_max;
	eff->pant.tiemp = 0;
	eff->pant.transp = transp;
}

void vfx_destello(short r, short g, short b, float transp, short tiemp_max) {
	Efectos_esp* eff{ NULL };
	eff = generar_efecto_fin();
	eff->tipo = 1;

	eff->pant.r = r;
	eff->pant.g = g;
	eff->pant.b = b;
	eff->pant.tiemp_max = tiemp_max;
	eff->pant.tiemp = 0;
	eff->pant.transp = transp;
}

void vfx_explosion_petacereza(short x, short y) {
	Efectos_esp* eff{ NULL };
	eff = generar_efecto_ini();
	eff->tipo = 3;

	eff->expl.x = x;
	eff->expl.y = y;
	eff->expl.tiemp = 0;
	eff->expl.aleatorio = rand() % 360;
}

void vfx_explosion_petaseta(short x, short y) {
	Efectos_esp* eff{ NULL };
	eff = generar_efecto_ini();
	eff->tipo = 4;

	eff->expl.x = x;
	eff->expl.y = y;
	eff->expl.tiemp = 0;
	eff->expl.aleatorio = rand() % 2;
}

Efectos_esp* generar_efecto_fin() {
	Efectos_esp* fin{ NULL }, * nuevo_vfx{ NULL };
	fin = vfx;
	while (fin->sig) {
		fin = fin->sig;
	}
	nuevo_vfx = new Efectos_esp;
	fin->sig = nuevo_vfx;
	nuevo_vfx->ant = fin;
	nuevo_vfx->sig = NULL;

	std::cout << "NUEVO EFFECTO: " << nuevo_vfx << std::endl;

	return nuevo_vfx;
}

Efectos_esp* generar_efecto_ini() {
	Efectos_esp* nuevo_vfx{ NULL };
	nuevo_vfx = new Efectos_esp;

	nuevo_vfx->sig = vfx->sig;
	nuevo_vfx->ant = vfx;

	if (vfx->sig) {
		vfx->sig->ant = nuevo_vfx;
	}
	vfx->sig = nuevo_vfx;

	std::cout << "NUEVO EFFECTO: " << nuevo_vfx << std::endl;

	return nuevo_vfx;
}

/*----------FUNCIONES-EXTRA-----------------------------------------------------------------------------------------------*/

void funcion_semillero() {
	for (int i{}; i < LIM_SEM; i++) {
		if (semillero[i].recarga > 0) {
			semillero[i].recarga--;
		}
	}
}

void cambiar_pantalla_ventana() {
	ALLEGRO_MONITOR_INFO resol_pant;
	al_get_monitor_info(0, &resol_pant);
	resol_x = resol_pant.x2 - resol_pant.x1;
	resol_y = resol_pant.y2 - resol_pant.y1;

	resol_x *= .6;
	resol_y *= .6;

	tam_pant_x = float(resol_x) / RESOL_X;
	tam_pant_y = float(resol_y) / RESOL_Y;
}

void cambiar_pantalla_completa() {
	ALLEGRO_MONITOR_INFO resol_pant;
	pantalla_completa = !pantalla_completa;
	if (pantalla_completa) {
		al_get_monitor_info(0, &resol_pant);
		resol_x = resol_pant.x2 - resol_pant.x1;
		resol_y = resol_pant.y2 - resol_pant.y1;
		tam_pant_x = float(resol_x) / RESOL_X;
		tam_pant_y = float(resol_y) / RESOL_Y;
	}
	else {
		cambiar_pantalla_ventana();
	}
	al_set_display_flag((ALLEGRO_DISPLAY*)b.al.display, ALLEGRO_FULLSCREEN_WINDOW, pantalla_completa);
}

void reescalar_pantalla() {
	//Dibujar transición cerrar juego
	if (mouse.tiemp_esc > 0) {
		float mult_ec = (float(RESOL_Y) / (2 * TIEMPO_ESC)) * -1;
		float apagado = float(mouse.tiemp_esc) / TIEMPO_ESC;
		float corte_y = (sqrt(pow(TIEMPO_ESC, 2) - pow(mouse.tiemp_esc, 2)) + TIEMPO_ESC) * mult_ec + RESOL_Y;
		corte_y *= 1.6;
		if (corte_y > RESOL_Y / 2) {
			corte_y = RESOL_Y / 2;
		}
		dibujar_texto((char*)"CERRANDO JUEGO", RESOL_X / 2 - 150, RESOL_Y / 2 - 15, M_TRANS_C(256 * apagado));

		al_set_target_backbuffer((ALLEGRO_DISPLAY*)b.al.display);
		al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.fondo_plantas, 0, 0, RESOL_X * 2, RESOL_Y * 2, M_TRANS_C(256 * apagado), 0, 0, -resol_x + int(mouse.tiemp_esc * mult_ec * -1.5) % resol_x, -resol_y + int(mouse.tiemp_esc * mult_ec * -1.5) % resol_y, tam_pant_x, tam_pant_y, 0, 0);

		//Dibujar reesclado
		al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.al.buffer, 0, corte_y, RESOL_X, RESOL_Y - corte_y * 2, M_TRANS_C(128 * (1.0 - apagado) + 128), 0, 0, 0, corte_y * tam_pant_y, tam_pant_x, tam_pant_y, 0, 0);
	}
	//Dibujar pantalla
	else {
		//Dibujar reesclado
		al_set_target_backbuffer((ALLEGRO_DISPLAY*)b.al.display);
		al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.al.buffer, 0, 0, RESOL_X, RESOL_Y, NORM_C, 0, 0, 0, 0, tam_pant_x, tam_pant_y, 0, 0);
	}
}

void finalizar_ejecucion() {
	if (mouse.tiemp_esc > 0) {
		mouse.tiemp_esc++;
		if (mouse.tiemp_esc >= TIEMPO_ESC) {
			bucle = false;
			programa_corriendo = false;
		}
	}
}

/*----------OPERACIONES-JUEGO---------------------------------------------------------------------------------------------*/

void inicializar_titulo() {
	b.titulo_bitmap = al_load_bitmap("Sprites/Start_Menu_bitmap.png");
	b.notas_bitmap = al_load_bitmap("Sprites/Extra/Notes_bitmap.png");

	cargar_record();

	bucle = true;
}

void finalizar_titulo() {
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.titulo_bitmap);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.notas_bitmap);
	delete[] tabla;
}

void inicializar_selector() {
	pos_semillero = 0;
	for (int i{}; i < LIM_SEM; i++) {
		semillero[i].plant = -2;
		semillero[i].recarga = 0;
		semillero[i].aleat = false;
	}

	soles_guard = 50;
	soles_guard_suma = 0;

	b.semillero_bitmap = al_load_bitmap("Sprites/Extra/Seedpackets_Bitmap.png");
	b.selector_hud = al_load_bitmap("Sprites/Extra/Seed_selection_hud.png");
	b.fondo_casa_dia = al_load_bitmap("Sprites/Daylight_Playground.png");
	b.fondo_casa_noche = al_load_bitmap("Sprites/Nighttime_Playground.png");
	b.hud_juego_bitmap = al_load_bitmap("Sprites/Extra/Game_Hud_Bitmap.png");
	b.sol_bitmap = al_load_bitmap("Sprites/Extra/Sun_Bitmap.png");
	b.texto_inicio_part = al_load_bitmap("Sprites/Extra/Start_Text_Bitmap.png");

	bucle = true;
}

void finalizar_selector() {
	for (int i{ pos_semillero }; i < LIM_SEM; i++) {
		semillero[i].aleat = true;
		semillero[i].plant = rand() % 8 + 1;
		if (i == 0) {
			semillero[i].plant = 2;
		}
	}

	al_destroy_bitmap((ALLEGRO_BITMAP*)b.selector_hud);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.semillero_bitmap);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.fondo_casa_dia);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.fondo_casa_noche);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.hud_juego_bitmap);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.sol_bitmap);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.texto_inicio_part);
}

void inicializar_juego() {
	//Inicializar soles
	sol_tablero = new Sol;
	sol_tablero->ant_sol = NULL;
	sol_tablero->sig_sol = NULL;
	sol_tablero->estado_act = 4;
	cant_sol_tablero = 0;

	//Inicializar partículas
	for (int i{}; i < CAS_Y; i++) {
		particulas[i] = new Particula;
		particulas[i]->ant_part = NULL;
		particulas[i]->sig_part = NULL;
		particulas[i]->id = -1;
		cant_particulas[i] = 0;
	}

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

	//inicializar Oleadas
	oleada.zombie_ataq = 0;
	oleada.tiemp_no_zomb = 0;
	oleada.dificultad = 1;
	oleada.lim_dificultad = 120;
	oleada.tiempo_oleada = FRAMES_INI;
	oleada.ritmo_nivel = FRAMES_INI * 3;
	oleada.lim_tiempo_oleada = 300;
	oleada.puntos_sum = 0;
	oleada.tiempo = 0;

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
			explosion_tablero[x][y].danio = 0;
			explosion_tablero[x][y].tipo_area = 0;
		}
	}

	frames = 1;
	frames_indep = 0;
	cant_zombi_elim = 0;
	cant_sol_recol = 0;
	soles_guard = 50;
	planta_elegida = 0;
	anim_sobre_tablero = 0;
	semillero_elegido = -2;
	planta_elegida = -2;

	pausa_total = false;
	fin_juego = false;
	mouse.opc_sel = OPC_NULL;

	b.sombra_buffer = al_create_bitmap(RESOL_X, RESOL_Y);
	b.objetos_buffer = al_create_bitmap(RESOL_X, RESOL_Y);

	b.plantas_dia = al_load_bitmap("Sprites/Plants/Plants_Daytime.png");
	b.plantas_noche = al_load_bitmap("Sprites/Plants/Plants_Nighttime.png");
	b.chomper_anim = al_load_bitmap("Sprites/Plants/Chomper_Bite.png");
	b.semillero_bitmap = al_load_bitmap("Sprites/Extra/Seedpackets_Bitmap.png");
	b.hud_juego_bitmap = al_load_bitmap("Sprites/Extra/Game_Hud_Bitmap.png");
	b.hud_pausa_bitmap = al_load_bitmap("Sprites/Extra/Pause_Hud_Bitmap.png");
	b.sol_bitmap = al_load_bitmap("Sprites/Extra/Sun_Bitmap.png");
	b.explosion = al_load_bitmap("Sprites/Extra/Explosion_Bitmap.png");
	b.guisante = al_load_bitmap("Sprites/Bullets/Bullet_Pea.png");
	b.zombie_bitmap = al_load_bitmap("Sprites/Zombies/Zombie_Basic.png");
	b.fondo_casa_dia = al_load_bitmap("Sprites/Daylight_Playground.png");
	b.fondo_casa_noche = al_load_bitmap("Sprites/Nighttime_Playground.png");
	b.enfoque_oscuro = al_load_bitmap("Sprites/Focus_Screen.png");
	b.texto_inicio_part = al_load_bitmap("Sprites/Extra/Start_Text_Bitmap.png");
	b.pantalla_blanca = al_load_bitmap("Sprites/Extra/White_Screen.png");
	b.hipno_fondo = al_load_bitmap("Sprites/Extra/Psychedelic_Screen.png");

	b.al.hud_buffer = al_create_bitmap(RESOL_X, RESOL_Y);

	bucle = true;
}

void finalizar_juego() {
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
		for (int y{}; y < CAS_Y; y++) {
			Particula* elim_part = NULL, * sig_elim_part = NULL;
			elim_part = particulas[y];
			for (int i{}; i < cant_particulas[y]; i++) {
				sig_elim_part = elim_part->sig_part;
				delete elim_part;
				std::cout << "POSICION " << i << " ELIMINADA" << std::endl;
				elim_part = sig_elim_part;
			}
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

	guardar.tiempo = frames;

	frames = 0;

	al_destroy_bitmap((ALLEGRO_BITMAP*)b.sombra_buffer);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.objetos_buffer);

	al_destroy_bitmap((ALLEGRO_BITMAP*)b.fondo_casa_dia);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.chomper_anim);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.plantas_dia);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.fondo_casa_noche);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.semillero_bitmap);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.hud_juego_bitmap);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.hud_pausa_bitmap);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.sol_bitmap);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.explosion);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.guisante);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.zombie_bitmap);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.enfoque_oscuro);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.texto_inicio_part);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.hipno_fondo);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.al.hud_buffer);
}

void inicializar_fin_juego() {
	b.enfoque_oscuro = al_load_bitmap("Sprites/Focus_Screen.png");
	b.texto_inicio_part = al_load_bitmap("Sprites/Extra/Start_Text_Bitmap.png");
	al_set_display_icon((ALLEGRO_DISPLAY*)b.al.display, al_load_bitmap("Sprites/Icon_OG.png"));

	cargar_record();

	strcpy(guardar.nombre, "AAA");
	frames = 1;
	indice_nombre = 0;
	nom_compl = false;

	bucle = true;
}

void finalizar_fin_juego() {
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.enfoque_oscuro);
	al_destroy_bitmap((ALLEGRO_BITMAP*)b.texto_inicio_part);
	al_set_display_icon((ALLEGRO_DISPLAY*)b.al.display, al_load_bitmap("Sprites/Icon.png"));

	//Guardar record
	guardar.puntos = oleada.puntos;
	guardar_record(guardar);
	delete[] tabla;

	oleada.puntos = 0;
	fin_juego = false;
	pausa_total = false;

	frames = 0;
}

/*----------------------DIBUJADO------------------------------------------------------------------------------------------*/

void dibujar_numero(long long num, float x, float y, ALLEGRO_COLOR color) {
	short tam{};
	long long copi_num{ num };
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

void dibujar_texto(char* text, short x, short y, ALLEGRO_COLOR color) {
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

void dibujar_record(float x, float y) {
	const int ESP_TIEMP{ 360 };
	for (int i{ 0 }; i < CANT_REC; i++) {
		dibujar_numero(tabla[i].puntos, x, y + 50 * i, NORM_C);
		dibujar_texto(tabla[i].nombre, x + 185, y + 50 * i, NORM_C);
		dibujar_numero((tabla[i].tiempo / 60) % 60, x + ESP_TIEMP + 56, y + 50 * i, NORM_C);
		dibujar_numero(tabla[i].tiempo / 3600, x + ESP_TIEMP, y + 50 * i, NORM_C);
		dibujar_texto((char*)".", x + ESP_TIEMP + 24, y + 50 * i, NORM_C);
		dibujar_texto((char*)".", x + ESP_TIEMP + 24, y + 50 * i - 15, NORM_C);
	}
}

void dibujo_planta_sombra(Planta& plant, short x, short y) {
	float tam{ 1.0 };
	short off_x = 195 + plant.dx;
	short off_y = 178 + plant.dy;

	//Recolocar sombra
	switch (plant.pos) {
	case 2: //Girasol
		off_y += 4;
		break;
	case 4: //Nuez
		off_y -= 2;
		break;
	case 7: //Carroñívora
		off_x -= 9;
		off_y += 1;
		break;
	case 9: //Desesporada
		off_x -= 9;
		off_y -= 1;
		tam = 0.9;
		break;
	case 10: //S. Solar
		if (plant.estado <= 1) {
			off_x -= 2;
			off_y += 2;
			tam = 0.7;
		}
		else {
			off_x -= 1;
			off_y -= 5;
			tam = 1.15;
		}
		break;
	case 11: //Humoseta
		off_x -= 2;
		off_y -= 4;
		break;
	case 13: //Hipnoseta
		off_y += 2;
		break;
	case 14: //Miédica
		off_x -= 1;
		off_y += 6;
		tam = 0.7;
		break;
	case 16: //Petaseta
		off_y += 4;
		break;
	case 5: case 12:
		return;
	}

	al_set_target_bitmap((ALLEGRO_BITMAP*)b.sombra_buffer);
	al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.plantas_dia, 0, 1350, 90, 90, NORM_C, 0, 0, x * 100 + off_x + 45 * (1.0 - tam), y * 100 + off_y + 45 * (1.0 - tam), tam, tam, 0, 0);
	al_set_target_bitmap((ALLEGRO_BITMAP*)b.objetos_buffer);
}

void dibujar_titulo() {
	al_set_target_bitmap((ALLEGRO_BITMAP*)b.al.buffer);
	//Efecto PARALLAX
	al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.titulo_bitmap, 0, 0, 1280, 720, NORM_C, 640, 360, 640 + float(mouse.x) * 32 / RESOL_X - 16, 360 + float(mouse.y) * 18 / RESOL_Y - 9, 1.025, 1.025, 0, 0);

	//Dibujar frente
	al_draw_bitmap_region((ALLEGRO_BITMAP*)b.titulo_bitmap, 0, 720, 1280, 720, 0, 0, 0);

	//Dibujar animación mano emergiendo
	if (tr.id == 3) {
		mouse.estado = 0;
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.titulo_bitmap, 1280, 788, 450, 788, 142, tr.mano.y + 100, 0);
		tr.mano.y /= 1.25;
		tr.mano.tiemp++;
	}

	if (mouse.estado == 1) {
		mouse.estado = 0;
	}

	//Dibujar botones
	if (mouse.x >= 836 && mouse.x < 1115 && mouse.y >= 198 && mouse.y < 368) {
		switch (mouse.estado) {
		default:mouse.estado = 1;	goto dibujar_boton_inicio;
		case 2:al_draw_bitmap_region((ALLEGRO_BITMAP*)b.titulo_bitmap, 1560, 0, 279, 174, 836, 196, 0);	break;
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

	//Dibujar marcador
	if (tr.y_global != 0) {
		float marcador_y;
		float extra{ 0 };
		if (tr.y_global > 1) {
			tr.y_global /= 1.125;
			if (tr.y_global <= 1) {
				tr.y_global = 1;
			}
		}
		if (tr.y_global < -1) {
			tr.y_global /= 1.125;
			extra = 1130 + tr.y_global;
			if (tr.y_global >= -1) {
				tr.y_global = 0;
			}
		}
		marcador_y = 190 + tr.y_global + extra;
		al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.notas_bitmap, 0, 0, 849, 640, NORM_C, 0, 0, 49, marcador_y - 122, .92, .92, 0, 0);
		dibujar_texto((char*)"puntuaci'on  nombre  tiempo", 130, marcador_y + 85, NORM_C);
		dibujar_record(240, marcador_y + 145);
	}

	//Animación arbustos
	if (tr.id == 3 && tr.mano.tiemp > 100) {
		al_draw_bitmap((ALLEGRO_BITMAP*)b.arbustos_transicion, 0, tr.mano.arbust_y, 0);
		//Terminar animación y pasar a la siguiente pantalla
		if (tr.mano.arbust_y <= -180) {
			tr.finalizado = true;
		}
		else {
			//Avanzar
			tr.mano.arbust_y -= RESOL_Y / 30;
		}
	}
	if (tr.id == 1) {
		mouse.estado = 0;
		al_draw_bitmap((ALLEGRO_BITMAP*)b.arbustos_transicion, 0, tr.arbustos.y, 0);
		if (tr.arbustos.y > -1080) {
			tr.arbustos.y -= RESOL_Y / 30;
		}
		else {
			tr.id = -1;
		}
	}
	dibujar_cursor();

	reescalar_pantalla();

	al_flip_display();
}

void dibujar_selector() {
	ALLEGRO_COLOR color_aux;
	short extr_x{ 0 }, mov_x{ 0 };
	al_set_target_bitmap((ALLEGRO_BITMAP*)b.al.buffer);

	//Asignar valores en transición a Juego
	if (tr.id == 2) {
		extr_x = tr.juego_ini.x;
		mov_x = tr.juego_ini.hud_x;
	}

	switch (tipo_patio) {
	case 0:	al_draw_bitmap((ALLEGRO_BITMAP*)b.fondo_casa_dia, -380 + extr_x, 0, 0);	break;
	case 1:	al_draw_bitmap((ALLEGRO_BITMAP*)b.fondo_casa_noche, -380 + extr_x, 0, 0);	break;
	}
	if (tr.y_global > 0) {
		switch ((tipo_patio + 1) % 2) {
		case 0:	al_draw_tinted_bitmap((ALLEGRO_BITMAP*)b.fondo_casa_dia, M_TRANS_C((int)tr.y_global), -380 + extr_x, 0, 0);	break;
		case 1:	al_draw_tinted_bitmap((ALLEGRO_BITMAP*)b.fondo_casa_noche, M_TRANS_C((int)tr.y_global), -380 + extr_x, 0, 0);	break;
		}
		tr.y_global /= 1.125;
		if (int(tr.y_global) == 0) {
			tr.y_global = 0;
		}
	}

	al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 0, 0, 940, 280, 15 + mov_x, 165, 0);

	if (mouse.estado == 1) {
		mouse.estado = 0;
	}

	//Dibujar botones
	//Botón iniciar ronda
	if (mouse.x >= 45 && mouse.x < 265 && mouse.y >= 445 && mouse.y < 575 && mouse.estado == 2) {
		//Presionado
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 0, 410, 220, 130, 45 + mov_x, 445, 0);
	}
	else {
		//Suelto
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 0, 280, 220, 130, 45 + mov_x, 445, 0);
	}

	//Botón Reiniciar selección
	if (pos_semillero > 0) {
		if (mouse.x >= 270 && mouse.x < 490 && mouse.y >= 445 && mouse.y < 575 && mouse.estado == 2) {
			//Presionado
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 220, 410, 220, 130, 270 + mov_x, 445, 0);
		}
		else {
			//Suelto
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 220, 280, 220, 130, 270 + mov_x, 445, 0);
		}
	}
	else {
		al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, M_NORM_C(102), 220, 280, 220, 130, 270 + mov_x, 445, 0);
	}

	//Botón Aleatorio
	if (mouse.x >= 495 && mouse.x < 620 && mouse.y >= 445 && mouse.y < 575 && mouse.estado == 2) {
		//Presionado
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 440, 405, 125, 131, 495 + mov_x, 445, 0);
	}
	else {
		//Suelto
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 440, 280, 125, 130, 495 + mov_x, 445, 0);
	}

	//Botón Día Noche
	if (mouse.x >= 630 && mouse.x < 755 && mouse.y >= 445 && mouse.y < 575 && mouse.estado == 2) {
		//Presionado
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 565, 540, 125, 131, 630 + mov_x, 445, 0);
	}
	else {
		//Suelto
		switch (tipo_patio % 2) {
		case 0:	al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 565, 280, 125, 130, 630 + mov_x, 445, 0);	break;
		case 1:	al_draw_bitmap_region((ALLEGRO_BITMAP*)b.selector_hud, 565, 410, 125, 130, 630 + mov_x, 445, 0);	break;
		}
	}

	//Dibujar semillero
	for (int i{ 0 }; i < LIM_SEM; i++) {
		ALLEGRO_COLOR color_costo;
		short costo_planta;
		costo_planta = COST_PLANTA[semillero[i].plant];
		color_costo = NORM_C;
		if ((tipo_patio + 1) % 2 && EXTR_SOL_DIA[semillero[i].plant] != 0) {
			costo_planta += EXTR_SOL_DIA[semillero[i].plant] * (1 - tr.y_global / 255);
			color_costo = al_map_rgb(236, 193, 94);
		}
		else if ((tipo_patio) % 2 && EXTR_SOL_DIA[semillero[i].plant] != 0) {
			costo_planta += EXTR_SOL_DIA[semillero[i].plant] * (tr.y_global / 255);
			color_costo = NORM_C;
		}
		if (semillero[i].plant != -2) {
			if (semillero[i].plant >= 1 && semillero[i].plant <= 8) {
				al_draw_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, 990, 0, 110, 110, i * 110 + 150, 0, 0);
				al_draw_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, (semillero[i].plant - 1) * 110, 0, 110, 110, i * 110 + 150, 0, 0);
				dibujar_numero(costo_planta, i * 110 + 215, 74, color_costo);
				if (mouse.x >= i * 110 + 150 && mouse.x < i * 110 + 260 && mouse.y <= 110) {
					//Cambiar sprite de cursor
					if (mouse.estado == 0)
						mouse.estado = 1;
				}
			}
			else if (semillero[i].plant >= 9 && semillero[i].plant <= 16) {
				al_draw_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, 990, 110, 110, 110, i * 110 + 150, 0, 0);
				al_draw_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, (semillero[i].plant - 9) * 110, 110, 110, 110, i * 110 + 150, 0, 0);
				dibujar_numero(costo_planta, i * 110 + 215, 74, color_costo);
				if (mouse.x >= i * 110 + 150 && mouse.x < i * 110 + 260 && mouse.y <= 110) {
					//Cambiar sprite de cursor
					if (mouse.estado == 0)
						mouse.estado = 1;
				}
			}
		}
		else {
			al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, TRANS_C, 880, 0, 110, 110, i * 110 + 150, 0, 0);
		}
	}

	//Dibujar Semillas
	for (int i{}; i < CANT_PLANT; i++) {
		ALLEGRO_COLOR color_costo;
		short costo_planta;
		bool en_semillero{ false };
		costo_planta = COST_PLANTA[i + 1];
		color_costo = NORM_C;
		if ((tipo_patio + 1) % 2 && EXTR_SOL_DIA[i + 1] != 0) {
			costo_planta += EXTR_SOL_DIA[i + 1] * (1 - tr.y_global / 255);
			color_costo = al_map_rgb(236, 193, 94);
		}
		else if ((tipo_patio) % 2 && EXTR_SOL_DIA[i + 1] != 0) {
			costo_planta += EXTR_SOL_DIA[i + 1] * (tr.y_global / 255);
			color_costo = NORM_C;
		}
		color_aux = NORM_C;
		for (int j{}; j < pos_semillero; j++) {
			if (semillero[j].plant == i + 1) {
				color_aux = al_map_rgb(64, 64, 64);
				color_costo.r *= .25;
				color_costo.b *= .25;
				color_costo.g *= .25;
				en_semillero = true;
			}
		}
		if (i >= 0 && i < 8) {
			al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, color_aux, 990, 0, 110, 110, i * 110 + 45 + mov_x, 190, 0);
			al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, color_aux, i * 110, 0, 110, 110, i * 110 + 45 + mov_x, 190, 0);
			dibujar_numero(costo_planta, i * 110 + 110 + mov_x, 264, color_costo);
			if (mouse.x >= i * 110 + 45 && mouse.x < i * 110 + 155 && mouse.y >= 190 && mouse.y < 300 && (pos_semillero < LIM_SEM || en_semillero)) {
				//Cambiar sprite de cursor
				if (mouse.estado == 0)
					mouse.estado = 1;
			}
		}
		else if (i >= 8 && i < CANT_PLANT) {
			al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, color_aux, 990, 110, 110, 110, (i - 8) * 110 + 45 + mov_x, 300, 0);
			al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, color_aux, (i - 8) * 110, 110, 110, 110, (i - 8) * 110 + 45 + mov_x, 300, 0);
			dibujar_numero(costo_planta, (i - 8) * 110 + 110 + mov_x, 374, color_costo);
			if (mouse.x >= (i - 8) * 110 + 45 && mouse.x < (i - 8) * 110 + 155 && mouse.y >= 300 && mouse.y < 410 && (pos_semillero < LIM_SEM || en_semillero)) {
				//Cambiar sprite de cursor
				if (mouse.estado == 0)
					mouse.estado = 1;
			}
		}
	}

	//Dibujar transiciones
	//Transición arbustos
	if (tr.id == 1) {
		mouse.estado = 0;
		al_draw_bitmap((ALLEGRO_BITMAP*)b.arbustos_transicion, 0, tr.arbustos.y, 0);
		if (tr.arbustos.y > -1080) {
			tr.arbustos.y -= RESOL_Y / 30;
		}
		else {
			tr.id = -1;
		}
	}
	//Transición a juego
	if (tr.id == 2) {
		mouse.estado = 0;
		//Mover fondo
		if (tr.juego_ini.x < 380) {
			tr.juego_ini.x *= 1.0625;
		}
		else if (tr.juego_ini.tiemp == 0) {
			//detener fondo
			tr.juego_ini.x = 380;
			tr.juego_ini.tiemp = 1;
		}
		//Mover HUD
		if (tr.juego_ini.hud_x < RESOL_X) {
			tr.juego_ini.hud_x *= 1.125;
		}
		//Si el fondo ya está en su posición correcta
		if (tr.juego_ini.tiemp > 0) {
			tr.juego_ini.tiemp++;
			//Sacar hud del juego normal
			if ((int)tr.juego_ini.hud_y > 0) {
				tr.juego_ini.hud_y /= 1.0625;
			}
			//Animación soles
			if (soles_guard_suma > 0 && (int)tr.juego_ini.hud_y <= 10) {
				soles_guard_suma--;
			}
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 0, 0, 130, 130, 10, -(int)tr.juego_ini.hud_y * 1.18, 0);
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hud_juego_bitmap, 0, 0, 110, 110, LIM_SEM * 110 + 150, -(int)tr.juego_ini.hud_y, 0);
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hud_juego_bitmap, 0, 110, 110, 110, RESOL_X - 110, -(int)tr.juego_ini.hud_y, 0);
			dibujar_numero(soles_guard - soles_guard_suma, 70, 90 - (int)tr.juego_ini.hud_y * 1.18, NORM_C);

			//Dibujar puntuación
			dibujar_texto((char*)"Puntuaci'on", 5, RESOL_Y - 85 + (int)tr.juego_ini.hud_y, NORM_C);
			dibujar_numero(oleada.puntos - oleada.puntos_sum, 110, RESOL_Y - 50 + (int)tr.juego_ini.hud_y, NORM_C);

			//Dibujar cronómetro
			dibujar_texto((char*)"Tiempo", RESOL_X - 160, RESOL_Y - 85 + (int)tr.juego_ini.hud_y, NORM_C);
			if ((frames / 60) % 60 >= 10) {	//Dibujar decenas
				dibujar_numero((frames / 60) % 60, RESOL_X - 60, RESOL_Y - 50 + (int)tr.juego_ini.hud_y, NORM_C);
			}
			else {	//Dibujar unidades
				al_draw_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, 0, 0, 30, 30, RESOL_X - 80, RESOL_Y - 50 + (int)tr.juego_ini.hud_y, 0);
				dibujar_numero((frames / 60) % 60, RESOL_X - 48, RESOL_Y - 50 + (int)tr.juego_ini.hud_y, NORM_C);
			}
			if (int(frames / 3600) >= 10) {	//Dibujar decenas
				dibujar_numero((frames / 3600), RESOL_X - 123, RESOL_Y - 50 + (int)tr.juego_ini.hud_y, NORM_C);
			}
			else {	//Dibujar unidades
				al_draw_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, 0, 0, 30, 30, RESOL_X - 143, RESOL_Y - 50 + (int)tr.juego_ini.hud_y, 0);
				dibujar_numero((frames / 3600), RESOL_X - 111, RESOL_Y - 50 + (int)tr.juego_ini.hud_y, NORM_C);
			}
			dibujar_texto((char*)".", RESOL_X - 95, RESOL_Y - 50 + (int)tr.juego_ini.hud_y, NORM_C);
			dibujar_texto((char*)".", RESOL_X - 95, RESOL_Y - 65 + (int)tr.juego_ini.hud_y, NORM_C);

			if (tr.juego_ini.tiemp > 270) {
				tr.finalizado = true;
			}
			else if (tr.juego_ini.tiemp > 220) {
				tr.juego_ini.text_act = 2;
			}
			else if (tr.juego_ini.tiemp > 170) {
				tr.juego_ini.text_act = 1;
			}
			else if (tr.juego_ini.tiemp > 120) {
				tr.juego_ini.text_act = 0;
			}

			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.texto_inicio_part, 0, tr.juego_ini.text_act * 540, 1082, 540, 80, 80, 0);
		}
	}

	dibujar_cursor();

	//Dibujar reesclado
	reescalar_pantalla();

	al_flip_display();
}

void dibujar_tablero() {
	ALLEGRO_COLOR color_aux;
	short sobre_dibujo{ 0 };
	bool enseniar_cursor{ true };

	//Iniciar Buffers vacíos
	al_set_target_bitmap((ALLEGRO_BITMAP*)b.sombra_buffer);
	al_clear_to_color(M_TRANS_C(0));
	al_set_target_bitmap((ALLEGRO_BITMAP*)b.objetos_buffer);
	al_clear_to_color(M_TRANS_C(0));

	//Dibujar plantas, proyectiles y zombies en el tablero
	for (int y{}; y < CAS_Y; y++) {
		Proyectil* ptr_proy = NULL, * ant_proy = NULL;
		Particula* ptr_part{ NULL }, * ant_part{ NULL };
		Zombie* ptr_zomb = NULL;

		//Dibujar cráter
		for (int x{ 0 }; x < CAS_X; x++) {
			if (planta[x][y].pos == -2) {
				al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.plantas_noche, NORM_C, 630, animacion_planta(x, y, false), 90, 90, x * 100 + 195, y * 100 + 165, 0);
			}
		}

		//Dibujar partículas
		ptr_part = particulas[y];
		for (int i{}; i < cant_particulas[y]; i++) {
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
			case 8://Ladrillo
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, 490, 975, 35, 32, color_aux, 6, 14, ptr_part->x + ptr_part->mov_x + 6, pos_y + 14, 1, 1, ptr_part->angulo * .01745, 0);
				break;
			}
		}

		//Dibujar plantas
		for (int x{}; x < CAS_X; x++) {
			if (planta[x][y].pos > 0) {
				char color = planta[x][y].anim_danio > 0 ? 255 - planta[x][y].anim_danio * 7 : 255;
				short off_x = 195 + planta[x][y].dx;
				short off_y = 165 + planta[x][y].dy;
				
				dibujo_planta_sombra(planta[x][y], x, y);

				switch (planta[x][y].pos) {
				case 10://Seta solar
					if (planta[x][y].estado > 2) {
						al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.plantas_noche, al_map_rgb(255, color, color), 720, animacion_planta(x, y, false), 90, 90, x * 100 + off_x, y * 100 + off_y, 0);
					}
					else {
						al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.plantas_noche, al_map_rgb(255, color, color), 90, animacion_planta(x, y, false), 90, 90, x * 100 + off_x, y * 100 + off_y, 0);
					}
					break;

				case 12://Calabaza
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.plantas_noche, al_map_rgb(255, color, color), 270, animacion_planta(x, y, true), 90, 90, x * 100 + off_x, y * 100 + off_y, 0);
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.plantas_noche, al_map_rgb(255, color, color), 270, animacion_planta(x, y, false), 90, 90, x * 100 + off_x, y * 100 + off_y, 0);
					break;

				case 13://Hipnoseta
					if (true) {	//if true usado para declaración de variables locales
						ALLEGRO_BITMAP* hipno_effect = al_create_bitmap(90, 90);		//BUSCAR UNA SOLUCIÓN PARA ESTA MEXICANADA
						float sx, sy;
						short corte_y = animacion_planta(x, y, false);
						short grados = pow(corte_y / 90 - 1, 1.25) * -1;
						sx = float((frames + planta[x][y].tiemp) % 800) / 4;
						sy = float(frames % 800) / 4;

						//Generar efecto hípnoseta
						al_set_target_bitmap(hipno_effect);
						al_clear_to_color(M_TRANS_C(0));
						al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hipno_fondo, sx, sy, 90, 90, 0, 0, 0);

						//Caso imagen no abarca todo el bitmap
						if (sx > 110) {
							al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hipno_fondo, 0, sy, 90, 90, 200 - sx, 0, 0);
						}
						if (sy > 110) {
							al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hipno_fondo, sx, 0, 90, 90, 0, 200 - sy, 0);
						}
						if (sx > 110 && sy > 110) {
							al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hipno_fondo, 0, 0, 90, 90, 200 - sx,  200 - sy, 0);
						}
						al_draw_bitmap(hipno_effect, 0, 0, (x + y) % 4);
						al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.plantas_noche, 360, 1350, 90, 90, M_TRANS_C(76), 0, 0, 0, 0, 1, 1, float(grados) * .01745, 0);

						//Recortar efecto
						al_set_blender(ALLEGRO_DEST_MINUS_SRC, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
						al_draw_bitmap_region((ALLEGRO_BITMAP*)b.plantas_noche, 360, corte_y + 630, 90, 90, 0, 0, 0);
						al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);

						//Dibujar hipnoseta
						al_set_target_bitmap((ALLEGRO_BITMAP*)b.al.buffer);
						al_draw_tinted_bitmap(hipno_effect, al_map_rgb(255, color, color), x * 100 + off_x, y * 100 + off_y, 0);
						al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.plantas_noche, al_map_rgb(255, color, color), 360, corte_y, 90, 90, x * 100 + off_x, y * 100 + off_y, 0);

						al_destroy_bitmap(hipno_effect);		//BUSCAR UNA SOLUCIÓN PARA ESTA MEXICANADA
					}
					break;

				default://Cualquier otra planta
					if (planta[x][y].pos >= 0 && planta[x][y].pos <= 8) {
						al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.plantas_dia, al_map_rgb(255, color, color), (planta[x][y].pos - 1) * 90, animacion_planta(x, y, false), 90, 90, x * 100 + off_x, y * 100 + off_y, 0);

					}
					else if (planta[x][y].pos >= 9 && planta[x][y].pos <= 16) {
						al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.plantas_noche, al_map_rgb(255, color, color), (planta[x][y].pos - 9) * 90, animacion_planta(x, y, false), 90, 90, x * 100 + off_x, y * 100 + off_y, 0);
					}
					break;
				}
				if (planta[x][y].anim_danio > 0) {		//Animación daño
					planta[x][y].anim_danio--;
				}
			}
		}

		//Dibujar zombies
		ptr_zomb = zombie[y];
		//Recorrer al fin de la lista
		while (ptr_zomb->sig_zomb != NULL) {
			//Se recorre al final de la lista enlazada
			ptr_zomb = ptr_zomb->sig_zomb;
		}
		for (int i{}; i < cant_zombi[y]; i++) {
			short color{ 0 };
			short color_r{ 255 }, color_g{ 255 }, color_b{ 255 };
			short estado_zomb{ 0 };
			float gorro_anim{ 0 }, gorro_danio{ 0 };

			//Animación de daño
			if (ptr_zomb->anim_danio > 0) {
				color = ptr_zomb->anim_danio > 0 ? 255 - ptr_zomb->anim_danio * 10 : 255;
				if (ptr_zomb->estado != 1)
					color_g = color_b = color;
				else
					color_g = color_r = color;
			}

			//Está congelado, se pinta de azul
			switch (ptr_zomb->estado) {
			case 1://Enfriado
				color_r -= 190;
				if (color_r < 0)
					color_r = 0;
				color_g -= 110;
				if (color_g < 0)
					color_g = 0;
				break;
			case 2://Congelado
				color_r -= 220;
				if (color_r < 0)
					color_r = 0;
				color_g -= 80;
				if (color_g < 0)
					color_g = 0;
				break;
			}

			//Determinar color del zombie
			color_aux = al_map_rgb(color_r, color_g, color_b);

			//Cambiar al sprite de daño
			if (ptr_zomb->est_danio == 1 || ptr_zomb->est_danio == -1) {
				estado_zomb = 135;
			}

			//Se mueve al zombie
			if (!pausa_total) {
				mover_zombie(*ptr_zomb, y);
			}

			//Dibujar sombra
			al_set_target_bitmap((ALLEGRO_BITMAP*)b.sombra_buffer);
			al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.plantas_dia, 0, 1350, 90, 90, NORM_C, 0, 0, ptr_zomb->x + 4, y * 100 + 177, 1.1, 1.1, 0, 0);
			al_set_target_bitmap((ALLEGRO_BITMAP*)b.objetos_buffer);

			//Se dibuja al zombie
			//Zombie comiendo
			if (ptr_zomb->comiendo) {
				estado_zomb = estado_zomb == 135 ? 648 : 0;
				al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, animacion_zombie(*ptr_zomb) + estado_zomb, 675, 108, 135, (int)ptr_zomb->x, y * 100 + 120, 0);
				if (ptr_zomb->est_danio >= 0) {
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, int(ptr_zomb->animacion / LIM_F_ZOMB_COM) % 4 * 108, 338, 108, 67, (int)ptr_zomb->x, y * 100 + 120, 0);
				}
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
					case 3://Ladrillo
						gorro_anim = int(ptr_zomb->animacion / LIM_F_ZOMB_COM) * .4;
						switch (int(ptr_zomb->animacion / LIM_F_ZOMB_COM) % 4) {
						case 0:			gorro_anim = 0;		break;
						case 1: case 3: gorro_anim = 0.6;	break;
						case 2:			gorro_anim = 1.4;	break;
						}
						switch (ptr_zomb->est_danio) {
						case 4: gorro_danio = 3; break;
						case 5: gorro_danio = 2; break;
						case 2: gorro_danio = 1; break;
						case 3: gorro_danio = 0; break;
						}
						al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, gorro_danio * 80 + 976, 927, 80, 80, color_aux, 0, 0, (int)ptr_zomb->x + 11, y * 100 + gorro_anim * 3 + 115, .85, .85, 0, 0);
						break;
					}
				}
			}
			//Zombie caminando
			else {
				switch (ptr_zomb->est_danio) {
				default://Normal
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, animacion_zombie(*ptr_zomb), estado_zomb, 108, 135, (int)ptr_zomb->x, y * 100 + 120, 0);
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, animacion_zombie(*ptr_zomb), 270, 108, 67, (int)ptr_zomb->x, y * 100 + 120, 0);
					break;
				//Animaciones de muerte
				case -1:
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, animacion_zombie(*ptr_zomb), estado_zomb, 108, 135, (int)ptr_zomb->x, y * 100 + 120, 0);
					break;
				case -2: case -3:
					if (ptr_zomb->est_danio == -3 && ptr_zomb->tiemp_muert > 24) {
						float trans = 1.0 - (ptr_zomb->tiemp_muert - 24) * .071;
						color_aux = al_map_rgba(color_r * trans, color_g * trans, color_b * trans, 255 * trans);
					}
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, color_aux, animacion_zombie(*ptr_zomb), 405, 108, 135, (int)ptr_zomb->x, y * 100 + 120, 0);
					break;
				case -4: case -5:
					if (ptr_zomb->tiemp_muert > 24) {
						float trans = 1.0 - (ptr_zomb->tiemp_muert - 24) * .071;
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
					case 3://Ladrillo
						gorro_anim = int(ptr_zomb->animacion / LIM_F_ZOMB) * .4;
						if (gorro_anim >= 3) {
							gorro_anim *= -1;
							gorro_anim += 5.5;
						}
						switch (ptr_zomb->est_danio) {
						case 4: gorro_danio = 3; break;
						case 5: gorro_danio = 2; break;
						case 2: gorro_danio = 1; break;
						case 3: gorro_danio = 0; break;
						}
						al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, gorro_danio * 80 + 976, 927, 80, 80, color_aux, 0, 0, (int)ptr_zomb->x + 11, y * 100 + gorro_anim * 3 + 108, .85, .85, 0, 0);
						break;
					}
				}
			}
			//Dibujar hielo bajo los pies
			if (ptr_zomb->estado == 2) {
				float tamanio;
				if (ptr_zomb->tiemp < 10) {
					tamanio = float(ptr_zomb->tiemp) / 10;
				}
				else {
					tamanio = 1;
				}
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.zombie_bitmap, 417, 968, 74, 38, M_TRANS_C(255 * tamanio), 0, 0, (int)ptr_zomb->x + 16, y * 100 + 220 + 38 * (1 - tamanio), 1, tamanio, 0, 0);
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
				if (!(frames % 6) && !pausa_total) {
					ptr_proy->tiempo_ev++;
				}
				switch (ptr_proy->tipo) {
				case -1: case -2://Guisantes
					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.guisante, ptr_proy->tiempo_ev * 28, ptr_proy->tipo * -28, 28, 28, ptr_proy->x, ptr_proy->y, 1);
					break;
				case -3://Espora desesporada
					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.guisante, ptr_proy->tiempo_ev * 28 + 112, 28, 28, 28, ptr_proy->x, ptr_proy->y, 1);
					break;
				case -4://Espora Miédica
					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.guisante, ptr_proy->tiempo_ev * 28 + 112, 28, 28, 28, ptr_proy->x, ptr_proy->y, 1);
					break;
				}
			}
			else {
				short movimiento = pausa_total ? 0 : rand() % 3;
				switch (ptr_proy->tipo) {
				case 0: case 1://Guisantes
					//Sombra
					al_set_target_bitmap((ALLEGRO_BITMAP*)b.sombra_buffer);
					al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.plantas_dia, 0, 1350, 90, 90, NORM_C, 0, 0, ptr_proy->x, ptr_proy->y + 32, 0.3, 0.3, 0, 0);
					al_set_target_bitmap((ALLEGRO_BITMAP*)b.objetos_buffer);

					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.guisante, ptr_proy->tipo * 28, 0, 28, 28, ptr_proy->x, ptr_proy->y + movimiento, 0);
					break;
				case 2://Esporas mini
					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.guisante, 112 + int(int(ptr_proy->x) % 120 / 40) * 28, 0, 28, 28, ptr_proy->x, ptr_proy->y + movimiento, 0);
					break;
				case 3://Esporas miédica
					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.guisante, 112 + int(int(ptr_proy->x) % 120 / 40) * 28, 0, 28, 28, ptr_proy->x, ptr_proy->y + movimiento, 0);
					break;
				case 4://Esporas Humoseta
					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.explosion, ptr_proy->tiempo_ev * 200, 500, 200, 200, ptr_proy->x + ptr_proy->esp.trasl_x, ptr_proy->y, 0);
					if (!(frames % 6) && !pausa_total) {
						ptr_proy->tiempo_ev++;
					}
					break;
				}
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
						short temblor_y = pausa_total ? 2 : rand() % 6;
						short temblor_x = pausa_total ? 2 : rand() % 6;
						trans = planta[x][y].tiemp > 10 ? 255 - (planta[x][y].tiemp - 10) * 100 : 255;
						color_aux = al_map_rgba(trans, trans, trans, trans);
						al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.explosion, color_aux, 1800, 300, 300, 200, x * 100 + 93 + temblor_x, y * 100 + 113 + temblor_y, 0);
					}
					sobre_dibujo++;
				}
				//Mordida chomper
				else if (planta[x][y].pos == 7 && (planta[x][y].estado == 1 || planta[x][y].estado == 2)) {
					char color = planta[x][y].anim_danio > 0 ? 255 - planta[x][y].anim_danio * 7 : 255;
					short off_x = x * 100 + 195 + planta[x][y].dx;
					short off_y = y * 100 + 120 + planta[x][y].dy;

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
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.chomper_anim, al_map_rgb(255, color, color), animacion_planta(x, y, true), 0, 180, 135, off_x, off_y, 0);
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
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 130, 0, 80, 80, TRANS_C, 40, 40, ptr_sol->x + ptr_sol->estado.anim.mov_x + 40, animacion_sol(*ptr_sol) + 40, ptr_sol->tam, ptr_sol->tam, (float)ptr_sol->angulo * .01745, 0);
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 210, 0, 80, 80, NORM_C, 40, 40, ptr_sol->x + ptr_sol->estado.anim.mov_x + 40, animacion_sol(*ptr_sol) + 40, ptr_sol->tam, ptr_sol->tam, 0, 0);
			}

			//Dibujar animación cayendo del cielo
			else if (ptr_sol->estado_act == -1) {
				if (ptr_sol->y < ptr_sol->estado.cayendo.mov_y && !pausa_total)
					ptr_sol->y += ptr_sol->estado.cayendo.mov_y / 500;
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 130, 0, 80, 80, TRANS_C, 40, 40, ptr_sol->x + 40, ptr_sol->y + 40, ptr_sol->tam, ptr_sol->tam, (float)ptr_sol->angulo * .01745, 0);
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 210, 0, 80, 80, NORM_C, 40, 40, ptr_sol->x + 40, ptr_sol->y + 40, ptr_sol->tam, ptr_sol->tam, 0, 0);
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
				tamanio *= ptr_sol->tam;
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 130, 0, 80, 80, M_TRANS_C(color * .8), 40, 40, ptr_sol->x + 40, ptr_sol->y + 40, tamanio, tamanio, (float)ptr_sol->angulo * .01745, 0);
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 210, 0, 80, 80, M_TRANS_C(color), 40, 40, ptr_sol->x + 40, ptr_sol->y + 40, ptr_sol->tam, ptr_sol->tam, 0, 0);
			}

			//Dibujar estado recolectado (va al sol de la interfaz)
			else {
				float escala, tamanio;
				//Mover al sol
				if (ptr_sol->x > 40 && ptr_sol->y > 10 && !pausa_total) {
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
				escala = (((float)ptr_sol->x + (float)ptr_sol->y) / (ptr_sol->estado.recol.mov_x + ptr_sol->estado.recol.mov_y) - .2) * ptr_sol->tam;
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 130, 0, 80, 80, TRANS_C, 40, 40, ptr_sol->x + 40 * escala, ptr_sol->y + 40 * escala, escala, escala, (float)ptr_sol->angulo * .01745, 0);
				al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 210, 0, 80, 80, NORM_C, 0, 0, ptr_sol->x, ptr_sol->y, escala, escala, 0, 0);
			}

			//Rotar el halo del sol
			if (frames % 2) {
				//Rota "clockwise"
				if (ptr_sol->angulo >= 0 && !pausa_total) {
					//Añade hasta los 360°
					if (ptr_sol->angulo < 360)
						ptr_sol->angulo++;
					else ptr_sol->angulo = 0;
				}
				//Rota "counterclockwise"
				else if (ptr_sol->angulo < 0 && !pausa_total) {
					//Añade hasta los -361°
					if (ptr_sol->angulo >= -360)
						ptr_sol->angulo--;
					else ptr_sol->angulo = -1;
				}
			}
		}
	}

	//Dibujar efectos especiales
	if (vfx->sig) {
		Efectos_esp* act{ NULL }, * ant{ NULL };
		ant = vfx;
		act = vfx->sig;
		while (act) {
			if (funcion_efecto(*act)) {
				act = ant;
			}
			ant = act;
			act = act->sig;
		}
	}

	al_set_target_bitmap((ALLEGRO_BITMAP*)b.al.buffer);

	switch (tipo_patio) {
	case 0:	al_draw_bitmap((ALLEGRO_BITMAP*)b.fondo_casa_dia, 0, 0, 0);		break;
	case 1:	al_draw_bitmap((ALLEGRO_BITMAP*)b.fondo_casa_noche, 0, 0, 0);	break;
	}

	al_draw_tinted_bitmap((ALLEGRO_BITMAP*)b.sombra_buffer, M_TRANS_C(96), 0, 0, 0);
	al_draw_bitmap((ALLEGRO_BITMAP*)b.objetos_buffer, 0, 0, 0);

	/*----------INTERFAZ------------------------------------------------------------------------------------------------------*/

	al_set_target_bitmap((ALLEGRO_BITMAP*)b.al.hud_buffer);
	al_clear_to_color(M_TRANS_C(0));

	//Dibuja la interfaz de pala
	al_draw_bitmap_region((ALLEGRO_BITMAP*)b.sol_bitmap, 0, 0, 130, 130, 10, 0, 0);
	al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hud_juego_bitmap, 0, 0, 110, 110, LIM_SEM * 110 + 150, 0, 0);

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
	dibujar_numero(soles_guard - soles_guard_suma, 70, 90, color_aux);

	//Dibujar semillero
	for (int i{ 0 }; i < LIM_SEM; i++) {
		ALLEGRO_COLOR color_costo;
		short costo_planta;
		costo_planta = COST_PLANTA[semillero[i].plant];
		color_costo = NORM_C;
		if ((tipo_patio + 1) % 2 && EXTR_SOL_DIA[semillero[i].plant] != 0) {
			costo_planta += EXTR_SOL_DIA[semillero[i].plant];
			color_costo = al_map_rgb(236, 193, 94);
		}
		if (semillero[i].plant != -2) {
			if (i != semillero_elegido) {
				color_costo = soles_guard >= costo_planta ? color_costo : al_map_rgb(255, 31, 95);
				//Dibujar fondo de la semilla
				if (semillero[i].aleat) {	//Fondo arcoíris
					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, 880, 220, 110, 110, i * 110 + 150, 0, 0);
				}							//Fondo Día Normal
				else if (semillero[i].plant >= 1 && semillero[i].plant <= 8) {
					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, 990, 0, 110, 110, i * 110 + 150, 0, 0);
				}							//Fondo Noche Normal
				else if (semillero[i].plant >= 9 && semillero[i].plant <= 16) {
					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, 990, 110, 110, 110, i * 110 + 150, 0, 0);
				}

				//Mostrar disponibilidad de plantas
				if (semillero[i].plant >= 1 && semillero[i].plant <= 8) {
					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, (semillero[i].plant - 1) * 110, 0, 110, 110, i * 110 + 150, 0, 0);
				}
				else if (semillero[i].plant >= 9 && semillero[i].plant <= 16) {
					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, (semillero[i].plant - 9) * 110, 110, 110, 110, i * 110 + 150, 0, 0);
				}

				//Mostrar cortina de recarga
				if (semillero[i].recarga > 0) {
					float porcent_recarga = (((float)semillero[i].recarga / REC_PLANTA[semillero[i].plant]) * 60 + 8);
					if (porcent_recarga > 110) {
						porcent_recarga = 110;
					}

					//Dibujar cortina arcoiris
					if (semillero[i].aleat) {
						al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, al_map_rgba(150, 135, 150, 211), 880, 220, 110, porcent_recarga, i * 110 + 150, 0, 0);
					}
					//Dibujar cortina gris
					else {
						al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, al_map_rgba(110, 95, 110, 211), 880, 110, 110, porcent_recarga, i * 110 + 150, 0, 0);
					}
				}
				dibujar_numero(costo_planta, i * 110 + 215, 74, color_costo);

			}
			else {
				//Planta seleccionada
				
				//Dibujar fondo de la semilla
				if (semillero[i].aleat) {
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, M_NORM_C(64), 880, 220, 110, 110, i * 110 + 150, 0, 0);
				}
				else if (semillero[i].plant >= 1 && semillero[i].plant <= 8) {
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, M_NORM_C(64), 990, 0, 110, 110, i * 110 + 150, 0, 0);
				}
				else if (semillero[i].plant >= 9 && semillero[i].plant <= 16) {
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, M_NORM_C(64), 990, 110, 110, 110, i * 110 + 150, 0, 0);
				}

				if (semillero[i].plant >= 1 && semillero[i].plant <= 8) {
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, M_NORM_C(64), (semillero[i].plant - 1) * 110, 0, 110, 110, i * 110 + 150, 0, 0);
				}
				else if (semillero[i].plant >= 9 && semillero[i].plant <= 16) {
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, M_NORM_C(64), (semillero[i].plant - 9) * 110, 110, 110, 110, i * 110 + 150, 0, 0);

				}
				color_costo.r *= .25;
				color_costo.b *= .25;
				color_costo.g *= .25;
				dibujar_numero(costo_planta, i * 110 + 215, 74, color_costo);
			}
		}
		else {
			al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.semillero_bitmap, TRANS_C, 880, 0, 110, 110, i * 110 + 150, 0, 0);
		}
	}

	//Pausa
	if (pausa_total && !fin_juego) {
		//Dibujar fondo
		short trasl_x = frames_indep % RESOL_X;
		short trasl_y = frames_indep % RESOL_Y;
		al_draw_tinted_bitmap((ALLEGRO_BITMAP*)b.fondo_plantas, TRANS_C, -RESOL_X + trasl_x, 0 - trasl_y, 0);

		//Dibujar menú
		al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.hud_pausa_bitmap, al_map_rgba(0, 0, 0, 140), 0, 0, 398, 487, 456, 130, 0);	//Sombra menú
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hud_pausa_bitmap, 0, 0, 398, 487, 441, 115, 0);									//Fondo menú

		//BOTONES PAUSA
		//Continuar
		if (mouse.x >= RESOL_X / 2 - 101 && mouse.x < RESOL_X / 2 + 101 && mouse.y >= 280 && mouse.y < 365 && mouse.estado == MOUSE_EST_CLICK) {
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hud_pausa_bitmap, 400, 100, 202, 94, RESOL_X / 2 - 101, 275, 0);	//Botón CONTINUAR Presionado
		}
		else {
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hud_pausa_bitmap, 400, 0, 202, 94, RESOL_X / 2 - 101, 275, 0);		//Botón CONTINUAR
		}

		//Reiniciar
		if (mouse.x >= RESOL_X / 2 - 101 && mouse.x < RESOL_X / 2 + 101 && mouse.y >= 375 && mouse.y < 460 && mouse.estado == MOUSE_EST_CLICK) {
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hud_pausa_bitmap, 605, 100, 202, 94, RESOL_X / 2 - 101, 370, 0);	//Botón REINICIAR
		}
		else {
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hud_pausa_bitmap, 605, 0, 202, 94, RESOL_X / 2 - 101, 370, 0);		//Botón REINICIAR
		}

		//Volumen
		if (mouse.x >= RESOL_X / 2 - 105 && mouse.x < RESOL_X / 2 - 5 && mouse.y >= 460 && mouse.y < 555 && mouse.estado == MOUSE_EST_CLICK) {
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hud_pausa_bitmap, 400, 300, 105, 100, RESOL_X / 2 - 105, 460, 0);	//Botón VOLUMEN
		}
		else {
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hud_pausa_bitmap, 400, 200, 105, 100, RESOL_X / 2 - 105, 460, 0);	//Botón VOLUMEN
		}

		//Casa
		if (mouse.x >= RESOL_X / 2 && mouse.x < RESOL_X / 2 + 100 && mouse.y >= 460 && mouse.y < 555 && mouse.estado == MOUSE_EST_CLICK) {
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hud_pausa_bitmap, 505, 300, 105, 100, RESOL_X / 2, 460, 0);			//Botón CASA
		}
		else {
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hud_pausa_bitmap, 505, 200, 105, 100, RESOL_X / 2, 460, 0);			//Botón CASA
		}
		dibujar_texto((char*)"PAUSA", RESOL_X / 2 - 55, 55, NORM_C);
	}

	//Dibujar puntuación actual
	dibujar_texto((char*)"Puntuaci'on", 5, RESOL_Y - 85, NORM_C);
	dibujar_numero(oleada.puntos - oleada.puntos_sum, 110, RESOL_Y - 50, NORM_C);
	if (oleada.puntos_sum > 0) {
		oleada.puntos_sum -= oleada.puntos_sum / 20 + 1;
	}

	//Dibujar cronómetro
	dibujar_texto((char*)"Tiempo", RESOL_X - 160, RESOL_Y - 85, NORM_C);
	if ((frames / 60) % 60 >= 10) {	//Dibujar decenas
		dibujar_numero((frames / 60) % 60, RESOL_X - 60, RESOL_Y - 50, NORM_C);
	}
	else {	//Dibujar unidades
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, 0, 0, 30, 30, RESOL_X - 80, RESOL_Y - 50, 0);
		dibujar_numero((frames / 60) % 60, RESOL_X - 48, RESOL_Y - 50, NORM_C);
	}
	if (int(frames / 3600) >= 10) {	//Dibujar decenas
		dibujar_numero((frames / 3600), RESOL_X - 123, RESOL_Y - 50, NORM_C);
	}
	else {	//Dibujar unidades
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, 0, 0, 30, 30, RESOL_X - 143, RESOL_Y - 50, 0);
		dibujar_numero((frames / 3600), RESOL_X - 111, RESOL_Y - 50, NORM_C);
	}
	dibujar_texto((char*)".", RESOL_X - 95, RESOL_Y - 50, NORM_C);
	dibujar_texto((char*)".", RESOL_X - 95, RESOL_Y - 65, NORM_C);

	//Dibujar Botón pausa
	if (mouse.x >= RESOL_X - 110 && mouse.x < RESOL_X && mouse.y > 0 && mouse.y < 110 && mouse.estado == MOUSE_EST_CLICK) {
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hud_juego_bitmap, 110, 110, 110, 110, RESOL_X - 110, 0, 0);
	}
	else {
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hud_juego_bitmap, 0, 110, 110, 110, RESOL_X - 110, 0, 0);
	}

	if (enseniar_cursor && !fin_juego) {
		dibujar_cursor();
	}
	
	//Animación muerte
	if (tr.id == 4) {
		short extra_y{ 0 }, text_extra_y{ 0 };
		short transpar, temblor_x, temblor_y;
		temblor_x = temblor_y = 0;

		//Mover foco de la imagen
		if (tr.muerte.tam > 1) {
			tr.muerte.tam /= 33.0 / 32;
		}
		else if (tr.muerte.tiemp == 0){
			tr.muerte.tam = 1;
			tr.muerte.tiemp = 1;
		}

		//Recorrer el foco de la imagen en Y
		if (tr.muerte.fila >= CAS_Y / 2) {
			extra_y = (CAS_Y / 2 - tr.muerte.fila) * 100.0 / 9;
		}
		else {
			extra_y = (2 - tr.muerte.fila) * 100.0 / 9;
		}

		//Aumentar tiempo
		if (tr.muerte.tiemp > 0) {
			tr.muerte.tiemp++;
		}

		//Aumentar tamaño texto fin
		if (tr.muerte.tiemp >= 60) {
			if (tr.muerte.tiemp == 60) {
				tr.muerte.tam_letr = 0.01;
			}
			if (tr.muerte.tam_letr < 1) {
				tr.muerte.tam_letr *= 1.125;
			}
			else {
				tr.muerte.tam_letr = 1;
			}
		}

		//Bajar temblorina
		if ((tr.muerte.tiemp - 60) % 60 == 0 && tr.muerte.tiemp - 60 > 0 && tr.muerte.temblor > 0) {
			tr.muerte.temblor--;
		}

		//Dibujar fundido a negro
		if (tr.muerte.tiemp > 480) {
			if (tr.muerte.tiemp <= 510) {
				transpar = 255 * (float(tr.muerte.tiemp - 480) / 30);
			}
			else {
				transpar = 255;
			}
			color_aux = al_map_rgba(transpar * .9, transpar * .9, transpar * .9, transpar);
			al_draw_tinted_scaled_rotated_bitmap((ALLEGRO_BITMAP*)b.enfoque_oscuro, color_aux, 640, 660, 640, 660, 10, 10, 0, 0);
		}

		transpar = 242 * ((10 - tr.muerte.tam) / 9);
		color_aux = al_map_rgba(transpar, transpar, transpar, transpar);

		//Efecto temblor texto
		if (tr.muerte.temblor != 0) {
			temblor_x = rand() % tr.muerte.temblor - tr.muerte.temblor / 2;
			temblor_y = rand() % tr.muerte.temblor - tr.muerte.temblor / 2;
		}
		al_draw_tinted_scaled_rotated_bitmap((ALLEGRO_BITMAP*)b.enfoque_oscuro, color_aux, 640, 660, tr.muerte.x + 640 * tr.muerte.tam - 180 * (tr.muerte.tam - 1), tr.muerte.y + 660 + extra_y * (tr.muerte.tam - 1), tr.muerte.tam, tr.muerte.tam, 0, 0);

		//Drsaparecer texto de fin
		if (tr.muerte.tiemp >= 600) {
			transpar = 255 * (1 - float(tr.muerte.tiemp - 600) / 90);
			//Cambiar de pantalla
			if (tr.muerte.tiemp >= 690) {
				transpar = 0;
				tr.finalizado = true;
			}
			color_aux = al_map_rgba(transpar, transpar, transpar, transpar);
		}
		else {
			color_aux = NORM_C;
		}

		al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.texto_inicio_part, 0, 1620, 1082, 540, color_aux, 541, 270, 621 + temblor_x, 350 + temblor_y + text_extra_y, tr.muerte.tam_letr, tr.muerte.tam_letr, 0, 0);
	}

	al_set_target_bitmap((ALLEGRO_BITMAP*)b.al.buffer);
	al_draw_bitmap((ALLEGRO_BITMAP*)b.al.hud_buffer, 0, 0, 0);

	//Dibujar reesclado
	reescalar_pantalla();

	//Dibujar en ventana
	al_flip_display();
}

void dibujar_fin_juego() {
	ALLEGRO_COLOR color_aux;
	short transp{ 255 }, text_y{ 0 }, trasl_x, trasl_y;
	short pos_x{ 280 };
	al_set_target_bitmap((ALLEGRO_BITMAP*)b.al.buffer);

	al_clear_to_color(al_map_rgb(0, 0, 0));

	//Transición
	if (tr.id == 4) {
		if (tr.muerte.tiemp < 180) {
			transp = 255 * (float(tr.muerte.tiemp) / 180);
		}
		else {
			transp = 255;
		}
		if (tr.muerte.tiemp >= 180) {
			tr.muerte.y /= 1.25;
		}
		text_y = tr.muerte.y;
		tr.muerte.tiemp++;
		if ((int)tr.muerte.y <= 1) {
			tr.id = 5;
			tr.finalizado = false;
			tr.fin.cant_sol_recol_anim = cant_sol_recol;
			tr.fin.cant_zombi_elim_anim = cant_zombi_elim;
			tr.fin.puntuacion_anim = oleada.puntos;
			tr.fin.tiemp = 0;
		}
	}
	trasl_x = frames % 1280;
	trasl_y = frames % 720;

	color_aux = al_map_rgba(transp, transp, transp, transp);
	al_draw_tinted_bitmap((ALLEGRO_BITMAP*)b.fondo_plantas, color_aux, -1280 + trasl_x, -720 + trasl_y, 0);
	al_draw_tinted_scaled_rotated_bitmap_region((ALLEGRO_BITMAP*)b.texto_inicio_part, 0, 2160, 1028, 540, NORM_C, 541, 270, RESOL_X / 2 + 541 * .5 - 1028 / 2 * .5, 20 + 270 * .5 - text_y, .5, .5, 0, 0);

	if (tr.id == 5) {
		if (tr.fin.tiemp > 60) {
			color_aux = al_map_rgb(145, 98, 200);
			dibujar_texto((char*)"zombis eliminados", pos_x, 380, color_aux);
			dibujar_numero(cant_zombi_elim - tr.fin.cant_zombi_elim_anim, pos_x + 670, 380, color_aux);
			if (tr.fin.cant_zombi_elim_anim > 0) {
				tr.fin.cant_zombi_elim_anim -= float(cant_zombi_elim) / 60;
			}
			else {
				tr.fin.cant_zombi_elim_anim = 0;
			}
		}
		if (tr.fin.tiemp > 180) {
			color_aux = al_map_rgb(255, 220, 42);
			dibujar_texto((char*)"soles recolectados", pos_x, 430, color_aux);
			dibujar_numero(cant_sol_recol - tr.fin.cant_sol_recol_anim, pos_x + 670, 430, color_aux);
			if (tr.fin.cant_sol_recol_anim > 0) {
				tr.fin.cant_sol_recol_anim -= float(cant_sol_recol) / 60;
			}
			else {
				tr.fin.cant_sol_recol_anim = 0;
			}
		}
		if (tr.fin.tiemp > 300) {
			dibujar_texto((char*)"puntuaci'on", pos_x, 480, NORM_C);
			dibujar_numero(oleada.puntos - tr.fin.puntuacion_anim, pos_x + 670, 480, NORM_C);
			if (tr.fin.puntuacion_anim > 0) {
				tr.fin.puntuacion_anim -= float(oleada.puntos) / 60;
			}
			else {
				tr.fin.puntuacion_anim = 0;
			}
		}
		if (tr.fin.tiemp > 420) {
			dibujar_texto((char*)"Nombre", pos_x, 530, NORM_C);
			al_draw_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, 180, 90, 30, 30, pos_x + 630 + indice_nombre % 3 * 22, 545, 0);
			dibujar_texto((char*)guardar.nombre, pos_x + 630, 530, NORM_C);
			if (nom_compl) {
				dibujar_texto((char*)"Pulse ENTER para salir", pos_x, 580, NORM_C);
			}
		}
		tr.fin.tiemp++;
	}
	else if (tr.id != 4) {
		color_aux = al_map_rgb(145, 98, 200);
		dibujar_texto((char*)"zombis eliminados", pos_x, 380, color_aux);
		dibujar_numero(cant_zombi_elim, pos_x + 670, 380, color_aux);

		color_aux = al_map_rgb(255, 220, 42);
		dibujar_texto((char*)"soles recolectados", pos_x, 430, color_aux);
		dibujar_numero(cant_sol_recol, pos_x + 670, 430, color_aux);

		dibujar_texto((char*)"puntuaci'on", pos_x, 480, NORM_C);
		dibujar_numero(oleada.puntos, pos_x + 670, 480, NORM_C);

		dibujar_texto((char*)"Nombre", pos_x, 530, NORM_C);
		al_draw_bitmap_region((ALLEGRO_BITMAP*)b.fuente_bitmap, 180, 90, 30, 30, pos_x + 630 + indice_nombre % 3 * 22, 545, 0);
		dibujar_texto((char*)guardar.nombre, pos_x + 630, 530, NORM_C);
		dibujar_texto((char*)"Pulse ENTER para salir", pos_x, 580, NORM_C);

	}

	if (tr.id == 1) {
		al_draw_bitmap((ALLEGRO_BITMAP*)b.arbustos_transicion, 0, tr.arbustos.y, 0);
		//Terminar animación y pasar a la siguiente pantalla
		if (tr.arbustos.y <= -180) {
			tr.finalizado = true;
		}
		else {
			//Avanzar
			tr.arbustos.y -= RESOL_Y / 30;
		}
	}

	//Dibujar reesclado
	reescalar_pantalla();

	//Dibujar en ventana
	al_flip_display();
}

void dibujar_cursor() {
	if (foto) {
		guardar_captura();
		foto = false;
		return;
	}
	switch (pantalla) {
	case 1://Selector
		if (tr.id == -1) {
			if (mouse.x >= 45 && mouse.x < 265 && mouse.y >= 445 && mouse.y < 575 && mouse.estado != 2) {
				mouse.estado = 1;
			}
			else if (mouse.x >= 270 && mouse.x < 490 && mouse.y >= 445 && mouse.y < 575 && mouse.estado != 2) {
				mouse.estado = 1;
			}
			else if (mouse.x >= 495 && mouse.x < 620 && mouse.y >= 445 && mouse.y < 575 && mouse.estado != 2) {
				mouse.estado = 1;
			}
			else if (mouse.x >= 630 && mouse.x < 755 && mouse.y >= 445 && mouse.y < 575 && mouse.estado != 2) {
				mouse.estado = 1;
			}
		}
		break;
	case 2://Juego
		//Regresar mouse al estado original si estaba apuntando
		if (mouse.estado != MOUSE_EST_CLICK) {
			mouse.estado = MOUSE_EST_NORMAL;
		}
		//Juego normal
		if (!pausa_total) {
			//Dibujar Planta elegida/ Pala
			if (planta_elegida) {
				if (planta_elegida >= 1 && planta_elegida <= 8) {
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.plantas_dia, al_map_rgba(165, 165, 165, 200), (planta_elegida - 1) * 90, 0, 90, 90, mouse.x - 45, mouse.y - 45, 0);
					return;
				}
				else if (planta_elegida >= 9 && planta_elegida <= 16) {
					al_draw_tinted_bitmap_region((ALLEGRO_BITMAP*)b.plantas_noche, al_map_rgba(165, 165, 165, 200), (planta_elegida - 9) * 90, 0, 90, 90, mouse.x - 45, mouse.y - 45, 0);
					return;
				}
				else if (planta_elegida == -1) {
					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hud_juego_bitmap, 110, 0, 110, 110, LIM_SEM * 110 + 150, 0, 0);
					al_draw_bitmap_region((ALLEGRO_BITMAP*)b.hud_juego_bitmap, 220, 0, 110, 110, mouse.x - 55, mouse.y - 55, 0);
					return;
				}
			}

			if (mouse.y < 110) {
				for (int i{ 0 }; i <= LIM_SEM; i++) {
					if (mouse.x >= 150 + i * 110 && mouse.x < 260 + i * 110) {
						if (i < LIM_SEM && semillero[i].recarga == 0 && semillero[i].plant >= 0) {
							if (mouse.estado == 0)
								mouse.estado = 1;
						}
						else if (i == LIM_SEM) {
							if (mouse.estado == 0)
								mouse.estado = 1;
						}
					}
				}
			}
			else {
				if (mouse.estado != MOUSE_EST_CLICK) {
					mouse.estado = MOUSE_EST_NORMAL;
				}
			}
		}
		//Menú pausa
		else {
			//Botón continuar
			if (mouse.x >= RESOL_X / 2 - 101 && mouse.x < RESOL_X / 2 + 101 &&
				mouse.y >= 280 && mouse.y < 365 && mouse.estado != MOUSE_EST_CLICK) {
				mouse.estado = MOUSE_EST_APUNTA;
			}
			//Botón reiniciar
			if (mouse.x >= RESOL_X / 2 - 101 && mouse.x < RESOL_X / 2 + 101 &&
				mouse.y >= 375 && mouse.y < 460 && mouse.estado != MOUSE_EST_CLICK) {
				mouse.estado = MOUSE_EST_APUNTA;
			}
			//Botón volumen
			if (mouse.x >= RESOL_X / 2 - 105 && mouse.x < RESOL_X / 2 - 5 &&
				mouse.y >= 460 && mouse.y < 555 && mouse.estado != MOUSE_EST_CLICK) {
				mouse.estado = MOUSE_EST_APUNTA;
			}
			//Botón casa
			if (mouse.x >= RESOL_X / 2 && mouse.x < RESOL_X / 2 + 100 &&
				mouse.y >= 460 && mouse.y < 555 && mouse.estado != MOUSE_EST_CLICK) {
				mouse.estado = MOUSE_EST_APUNTA;
			}
		}
		if (mouse.x >= RESOL_X - 110 && mouse.x < RESOL_X && mouse.estado != 2) {
			mouse.estado = MOUSE_EST_APUNTA;
		}
		break;
	}

	al_draw_bitmap_region((ALLEGRO_BITMAP*)b.cursor_bitmap, mouse.estado * 70, 0, 70, 70, mouse.x - 20, mouse.y - 20, 0);
}

 /*----------------------ARCHIVOS-----------------------------------------------------------------------------------------*/

void guardar_record(Record rec) {
	FILE* records, * temp;
	Record comp, ant;
	int cant{ 0 };
	bool ingresado{ false };
	records = fopen("marcadorPvZ.bin", "rb");
	temp = fopen("temp.bin", "wb");
	if (records) {
		while (!feof(records) && cant < CANT_REC) {
			fread(&comp, sizeof(Record), 1, records);
			//Si el nuevo récord es superior, se introduce
			if (comp.puntos < rec.puntos && !ingresado) {
				std::cout << "GUARDADO RECORD" << std::endl;
				fwrite(&rec, sizeof(Record), 1, temp);
				std::cout << rec.nombre << "\t" << rec.puntos << std::endl;
				cant++;
				ingresado = true;
				if (cant < CANT_REC) {
					fwrite(&comp, sizeof(Record), 1, temp);
					std::cout << comp.nombre << "\t" << comp.puntos << std::endl;
					cant++;
				}
			}
			else {
				if (cant > 0) {
					//Si se repite un récord, este se elimina
					if (ant.puntos != comp.puntos || strcmp(comp.nombre, ant.nombre) != 0) {
						fwrite(&comp, sizeof(Record), 1, temp);
						std::cout << comp.nombre << "\t" << comp.puntos << std::endl;
						cant++;
					}
				}
				else {
					//Copia
					fwrite(&comp, sizeof(Record), 1, temp);
					std::cout << comp.nombre << "\t" << comp.puntos << std::endl;
					cant++;
				}
			}
			ant = comp;
		}
		while (cant < CANT_REC) {
			strcpy(comp.nombre, "AAA");
			comp.puntos = 0;
			comp.tiempo = 0;
			fwrite(&comp, sizeof(Record), 1, temp);
			std::cout << comp.nombre << "\t" << comp.puntos << std::endl;
			cant++;
		}
		fclose(records);
		remove("marcadorPvZ.bin");
	}
	else {
		std::cout << "No se encontro" << std::endl;
		fwrite(&rec, sizeof(Record), 1, temp);
	}
	fclose(temp);
	rename("temp.bin", "marcadorPvZ.bin");
}

void cargar_record() {
	FILE* records;
	Record aux;
	int i{ 0 };
	tabla = new Record[CANT_REC + 1];
	records = fopen("marcadorPvZ.bin", "rb");
	if (records) {
		while (!feof(records)) {
			fread(&aux, sizeof(Record), 1, records);
			tabla[i] = aux;
			i++;
		}
		fclose(records);
	}
	strcpy(aux.nombre, "AAA");
	aux.puntos = 0;
	aux.tiempo = 0;
	while (i < CANT_REC) {
		tabla[i] = aux;
		i++;
	}
}

void guardar_captura() {
	char nombre[50];
	bool primera_iter{ true };
	int id{ 0 };
	FILE* aux{ NULL };
	for (id;  aux || primera_iter; id++) {
		if (aux) {
			fclose(aux);
		}
		sprintf(nombre, "Screenshots/Captura_%d.png", id);
		primera_iter = false;
		aux = fopen(nombre, "rb");
	}
	al_save_bitmap(nombre, (ALLEGRO_BITMAP*)b.al.buffer);
	if (aux) {
		fclose(aux);
	}
	foto = false;
}