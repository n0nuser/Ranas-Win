/*
   Fichero: ranas.cpp
   Integrantes del grupo:	Pablo Jesús González Rubio - i0894492
							Sergio García González - i0921911
   Fecha de modificación: 
 */

#include <stdio.h> 
#include <stdlib.h>
#include <Windows.h>
#include <iostream>
#include "ranas.h"
#include <time.h>

typedef BOOL DESTINO, *PDESTINO;
typedef HANDLE TRONCOS, *PTRONCOS, ORILLA, *PORILLA;

#define HOR_MIN 0
#define HOR_MAX 79
#define VER_MIN 0
#define VER_MAX 11 //Orilla
#define ORILLA_MIN 0
#define ORILLA_MAX 3
#define TRONCOS_MIN 4
#define TRONCOS_MAX 10
#define MSG_ERROR "GetProcAddress FERROR."
#define THREAD_MAX 10000


#define FERROR(ReturnValue,ErrorValue,ErrorMsg)							\
    do{																					\
        if((ReturnValue) == (ErrorValue)){												\
            fprintf(stderr, "\n[%d:%s] FERROR: %s", __LINE__, __FUNCTION__,ErrorMsg);	\
            exit(EXIT_FAILURE);															\
        }																				\
    }while(0)



struct funcionesDLL {
	HINSTANCE ranasDLL;
	TIPO_AVANCERANA avanceRana;
	TIPO_AVANCERANAINI avanceRanaIni;
	TIPO_AVANCERANAFIN avanceRanaFin;
	TIPO_AVANCETRONCOS avanceTroncos;
	TIPO_COMPROBARESTADISTICAS comprobarEstadisticas;
	TIPO_FINRANAS finRanas;
	TIPO_INICIORANAS inicioRanas;
	TIPO_PARTORANAS partoRanas;
	TIPO_PUEDOSALTAR puedoSaltar;
	
	TIPO_PAUSA pausa;
	TIPO_PRINTMSG printMSG;
} funciones;

//FUNCIONES
void tratarArg(int argc, char* argv[]);
int cargarRanas();
void f_criar(int pos);
DWORD WINAPI moverRanas(LPVOID lpParam);
void avanzarRana(int *posX, int *posY, int dir);


//VAR GLOBALES
long * nacidas, * salvadas, * perdidas;
int nMadre = 0;
int noTerminado = 1;
int posicion;
//Memoria Compartida
//Semáforos

//HILOS
HANDLE hilos[THREAD_MAX];
DWORD hilosID;

//MUTEXES DEBUG
HANDLE ranasMutex;

//MAIN Comentario
int main(int argc, char* argv[])
{
	system("mode con:cols=80 lines=25"); //FIJA AUTOMÁTICAMENTE A 80x25
	setlocale(LC_ALL, "");

	//PRIORIDAD DEL PROCESO NORMAL --> IDLE
	FERROR(SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS),FALSE,"SetPriorityClass");

	int velocidad, parto;
	int lTroncos[] = { 4,5,4,5,4,5,4 };
	int lAguas[] = { 5,4,3,5,3,4,5 };
	int dirs[] = { 1,0,1,0,1,0,1 };
	int i,* posX, * posY;
	
	//int nRana, nProcesos, nTroncos, param1, param2;
	int * movX, * movY; //Probar a quitar el global

	//TRATAMIENTO ARGUMENTOS
	tratarArg(argc, argv);
	velocidad = atoi(argv[1]);
	parto = atoi(argv[2]);

	FERROR(cargarRanas(),-1,"ERROR al cargar la biblioteca ranas.");

	//CREACIÓN MEMORIA COMPARTIDA

	//¿CREACIÓN PUNTERO MEMORIA COMPARTIDA?

	//INICIALIZACIÓN MEMORIA
	posX = (PINT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(posX) * 8);
	//Los posX y posY a lo mejor necesitamos inicializar tantas veces como hilos haya
	posY = (PINT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(posY) * 8 + 4);
	*posX = -1;
	*posY = -1;
	nacidas = (PLONG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(nacidas) * 8);
	salvadas = (PLONG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(salvadas) * 8);
	perdidas = (PLONG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(perdidas) * 8);
	*nacidas = *salvadas = *perdidas = 0;

	//CREACIÓN RECURSOS DEBUG
	FERROR(ranasMutex = CreateMutex(NULL, FALSE, NULL),NULL, "CreateMutex() ERROR.\n");

	funciones.inicioRanas(velocidad, lTroncos, lAguas, dirs, parto, f_criar);
	
	Sleep(30000); // Se debe esperar 30 segundos para finalizar el programa

	funciones.finRanas();

	//Acabar con hilos de ranas
	WaitForMultipleObjects(THREAD_MAX, hilos, TRUE, INFINITE);
	for (i = 0; i < THREAD_MAX; i++) CloseHandle(hilos[i]);
	CloseHandle(ranasMutex);

	funciones.comprobarEstadisticas(*nacidas, *salvadas, *perdidas);
	
	return 0;
}

// Funcion f_criar
void f_criar (int pos) { //Llamará a la función PartoRanas, actualiza las estadísticas, crea un nuevo hilo para mover a la recién nacida y mueve los troncos
	
	//pos = 0; //DEBUG (PARA QUE SOLO PARA LA RANA MADRE 0)
	int* movX, *movY;
	movX = (PINT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(movX) * 8);
	movY = (PINT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(movY) * 8 + 4); //Posiblemente quitar parametros 
	
	if (funciones.partoRanas(pos)) {
		nacidas++;
		posicion = pos;
		//ABRIR SEMÁFORO CONTROL DE HILOS
		FERROR(CreateThread(NULL, 0, moverRanas, 0, 0, NULL), NULL, "THREAD_CREATION ERROR");
	} else {
		Sleep(5000);
		printf("\nHa ocurrido un error en el parto de las ranas.\n");
		exit(777);
	}
	/*
	for (nTroncos = 0; nTroncos < 7; nTroncos++) {
		//ABRIR MEMORIA COMPARTIDA

		//AVANCE_TRONCOS

		for (nProcesos = 0; nProcesos < 25; nProcesos++)
		{
			movX = (int*)(ptr + 2048 + nProcesos * 8);
			movY = (int*)(ptr + 2048 + nProcesos * 8 + 4);

			if ((*movY) == 10 - nTroncos) {
				if (dirs[nTroncos] == 0)
					(*movX)++;
				else
					(*movX)--;
			}
		}
		//CIERRA MEMORIA COMPARTIDA

		//BATR_PAUSA
	}
	*/
}

DWORD WINAPI moverRanas(LPVOID lpParam) {
	int sentido;
	int * movX, * movY;
	int nacimiento = 0;
	DWORD mem1, mem2;


	//PUNTERO MEMORIA COMPARTIDA A CERO
	movX = (PINT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(movX) * 8);
	movY = (PINT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(movY) * 8 + 4); //Posiblemente quitar parametros

	*movX = 15 + 16 * posicion;
	*movY = 0;

	while (noTerminado) //Poner los noterminado=0
	{
		mem1 = WaitForSingleObject(ranasMutex, INFINITE); //DEBUG
		switch (mem1) {
			case WAIT_OBJECT_0:
					//INICIALIZAR PUNTEROS POSX POSY
					if ((*movX) < 0 || (*movX) > 79){
						FERROR(ReleaseMutex(ranasMutex), FALSE, "ReleaseMutex() ERROR.\n");

						(*perdidas)++;
						(*movY) = -1;
						(*movX) = -1;
						break;
					}
					
					if (funciones.puedoSaltar(*movX, *movY, ARRIBA)) sentido = ARRIBA;
					else if (funciones.puedoSaltar(*movX, *movY, DERECHA)) sentido = DERECHA;
					else if (funciones.puedoSaltar(*movX, *movY, IZQUIERDA)) sentido = IZQUIERDA;
					else {
						FERROR(ReleaseMutex(ranasMutex), FALSE, "ReleaseMutex() ERROR.\n");
						funciones.pausa();
						//printf("No puedo saltar");
						continue;
					}

					//METER LOS FERROR
					FERROR(funciones.avanceRanaIni(*movX, *movY),FALSE,"avanceRanaIni() ERROR.\n");
					FERROR(funciones.avanceRana(movX, movY, sentido), FALSE, "avanceRana() ERROR.\n");

					FERROR(ReleaseMutex(ranasMutex), FALSE, "ReleaseMutex() ERROR.\n");

					funciones.pausa();

					mem2 = WaitForSingleObject(ranasMutex, INFINITE);
					switch (mem2) {
						case WAIT_OBJECT_0:
							
							//INICIAR PUNTEROS MOVX MOVY

							if ((*movX) < 0 || (*movX) > 79)
							{
								FERROR(ReleaseMutex(ranasMutex), FALSE, "ReleaseMutex() ERROR.\n");

								(*perdidas)++;
								(*movY) = -1;
								(*movX) = -1;
								break;
							}

							FERROR(funciones.avanceRanaFin(*movX, *movY), FALSE, "avanceRanaFin() ERROR.\n");


							if ((*movY) == 11) {
								FERROR(ReleaseMutex(ranasMutex), FALSE, "ReleaseMutex() ERROR.\n");
								(*salvadas)++;
								(*movY) = -1;
								(*movX) = -1;
								break;
							}
							/*
							if ((*movY) == 1 && nacimiento == 0)
							{
								nacimiento = 1;
								sems.sem_num = nRana + 2;
								sems.sem_op = 1;
								sems.sem_flg = 0;
								if (semop(sem, &sems, 1) == -1) perror("\033[1;31mError semáforo de control de nacimiento de ranaMadre.\033[0m\n");
							}
							*/
							FERROR(ReleaseMutex(ranasMutex), FALSE, "ReleaseMutex() ERROR.\n");
						case WAIT_ABANDONED:
							return FALSE;
					}
			case WAIT_ABANDONED:
				return FALSE;
		}
	}
	//CERRAR SEMÁFORO CONTROL DE HILOS
	return TRUE;
}

/* ============= Función para tratar argumentos ============= */
void tratarArg(int argc, char * argv[]) {

int param1, param2;


if(argc != 3) {
	fprintf(stderr,"%s\n","USO: ./ranas VELOCIDAD VELOCIDAD_PARTO");
	exit(1);
}

param1 = atoi(argv[1]);
param2 = atoi(argv[2]);

if (param1 < 0 || param1 > 1000) {
	fprintf(stderr, "%s\n", "Ha introducido una velocidad incorrecta.\nPor favor, introduzca una velocidad ente 0 y 1000.");
	exit(1);
}

if (param2 <= 0) {
	fprintf(stderr, "%s\n", "Ha introducido un tiempo de partos incorrecto.\nIntroduzca un tiempo mayor de 0.");
	exit(1);
}

}

/* ============= Función para cargar la biblioteca (DLL) ============= */
int cargarRanas() {
	FERROR(funciones.ranasDLL = LoadLibrary(TEXT("ranas.dll")),NULL, "Error: LoadLibrary");
	FERROR(funciones.avanceRana = (TIPO_AVANCERANA)GetProcAddress(funciones.ranasDLL, "AvanceRana"), NULL, "Error: AvanceRana");
	FERROR(funciones.avanceRanaFin = (TIPO_AVANCERANAFIN) GetProcAddress(funciones.ranasDLL, "AvanceRanaFin"), NULL, "Error: AvanceRanaFin");
	FERROR(funciones.avanceRanaIni = (TIPO_AVANCERANAINI) GetProcAddress(funciones.ranasDLL, "AvanceRanaIni"), NULL, "Error: AvanceRanaIni");
	FERROR(funciones.avanceTroncos = (TIPO_AVANCETRONCOS) GetProcAddress(funciones.ranasDLL, "AvanceTroncos"), NULL, "Error: AvanceTroncos");
	FERROR(funciones.comprobarEstadisticas = (TIPO_COMPROBARESTADISTICAS) GetProcAddress(funciones.ranasDLL,"ComprobarEstadIsticas"), NULL, "Error: ComprobarEstadisticas");
	FERROR(funciones.finRanas = (TIPO_FINRANAS) GetProcAddress(funciones.ranasDLL, "FinRanas"), NULL, "Error: FinRanas");
	FERROR(funciones.inicioRanas = (TIPO_INICIORANAS)GetProcAddress(funciones.ranasDLL, "InicioRanas"), NULL, "Error: InicioRanas");
	FERROR(funciones.partoRanas = (TIPO_PARTORANAS) GetProcAddress(funciones.ranasDLL, "PartoRanas"), NULL, "Error: PartoRanas");
	FERROR(funciones.puedoSaltar = (TIPO_PUEDOSALTAR) GetProcAddress(funciones.ranasDLL, "PuedoSaltar"), NULL, "Error: PuedoSaltar");
	
	FERROR(funciones.pausa = (TIPO_PAUSA)GetProcAddress(funciones.ranasDLL, "Pausa"), NULL, "Error: Pausa");
	FERROR(funciones.printMSG = (TIPO_PRINTMSG)GetProcAddress(funciones.ranasDLL, "PrintMsg"), NULL, "Error: PrintMsg");

	return 0;
}