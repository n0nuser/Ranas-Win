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

typedef BOOL DESTINO, * PDESTINO;
typedef HANDLE TRONCOS, * PTRONCOS, ORILLA, * PORILLA;

#define HOR_MIN 0
#define HOR_MAX 79
#define VER_MIN 0
#define VER_MAX 11 //Orilla
#define ORILLA_MIN 0
#define ORILLA_MAX 3
#define TRONCOS_MIN 4
#define TRONCOS_MAX 10
#define INF 1000000


#define FERROR(ReturnValue,ErrorValue,ErrorMsg)							\
    do{																					\
        if((ReturnValue) == (ErrorValue)){												\
            fprintf(stderr, "\n[%d:%s] FERROR: %s", __LINE__, __FUNCTION__,ErrorMsg);	\
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
void criar(int pos);
DWORD WINAPI moverRanas(LPVOID lpParam);


//VAR GLOBALES
PLONG nacidas, salvadas, perdidas;
PINT pX, pY;
int posicion; //Se utiliza para pasarle "pos" a los movXY de moverRanas desde f_criar
int lTroncos[] = { 5,5,5,5,5,5,5 };
int lAguas[] = { 1,1,1,1,1,1,1 };
int dirs[] = { 1,1,1,1,1,1,1 };
int ini = 0, cont; //Las utilizamos para pasarle el tronco que se mueve a la ranita y cont para la dirección
						//Por ahora la posición está bloqueada para que se mueva a la izquierda

HANDLE semaforo_troncos;

//MUTEXES
HANDLE mu[80][12], control;
CRITICAL_SECTION sc;

/* ======================================= MAIN ======================================= */

int main(int argc, char* argv[])
{
	system("mode con:cols=80 lines=25"); //FIJA AUTOMÁTICAMENTE A 80x25
	setlocale(LC_ALL, "");
	FERROR(SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS), FALSE, "SetPriorityClass"); //PRIORIDAD DEL PROCESO NORMAL --> IDLE

	int velocidad, parto;
	int i, j;

	//TRATAMIENTO ARGUMENTOS
	tratarArg(argc, argv);
	velocidad = atoi(argv[1]);
	parto = atoi(argv[2]);

	//CARGA DE LAS FUNCIONES DE BIBLIOTECA
	FERROR(cargarRanas(), -1, "cargarRanas()\n");

	//INICIALIZACIÓN MEMORIA
	nacidas = (PLONG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PLONG));
	salvadas = (PLONG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PLONG));
	perdidas = (PLONG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PLONG));
	pX = (PINT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PINT));
	pY = (PINT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PINT));
	*nacidas = *salvadas = *perdidas = *pY = *pX = 0;

	//MATRIZ 80x25 MUTEX
	//DAR ESPACIO MEMORIA A HANDLE mu
	for (i = HOR_MIN; i <= HOR_MAX; i++)
		for (j = VER_MIN; j <= VER_MAX; j++)
			FERROR(mu[i][j] = (HANDLE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HANDLE)), NULL, "HeapAlloc()\n");
	//CARGAR HANDLE mu CON MUTEX
	for (i = HOR_MIN; i <= HOR_MAX; i++)
		for (j = VER_MIN; j <= VER_MAX; j++)
			FERROR(mu[i][j] = CreateMutex(NULL, FALSE, NULL), NULL, "CreateMutex()\n");

	FERROR(control = (HANDLE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HANDLE)), NULL, "HeapAlloc()\n");
	FERROR(control = CreateMutex(NULL, FALSE, NULL), NULL, "CreateMutex()\n");

	InitializeCriticalSection(&sc);

	semaforo_troncos = CreateSemaphore(NULL, 0, 1, "semaforo_troncos");

	//INICIO DEL PROGRAMA
	FERROR(funciones.inicioRanas(velocidad, lTroncos, lAguas, dirs, parto, criar), FALSE, "inicioRanas()\n");

	Sleep(30000); // Se debe esperar 30 segundos para finalizar el programa

	//CERRAMOS RECURSOS
	FERROR(funciones.finRanas(), NULL, "finRanas()\n");
	for (i = HOR_MIN; i <= HOR_MAX; i++)
		for (j = VER_MIN; j <= VER_MAX; j++)
			FERROR(CloseHandle(mu[i][j]), NULL, "CloseHandle()\n");

	DeleteCriticalSection(&sc);


	FERROR(funciones.comprobarEstadisticas(*nacidas, *salvadas, *perdidas), NULL, "comprobarEstadisticas()\n");

	return 0;
}

void criar(int pos) {
	//Para los troncos
	int nTroncos; //Controla qué tronco se mueve

	while (TRUE) {
		if (funciones.partoRanas(pos)) {
			nacidas++;
			posicion = pos;
			FERROR(CreateThread(NULL, 0, moverRanas, 0, 0, NULL), NULL, "CreateThread()\n");
		}
		else {
			printf("partoRanas()\n");
			exit(777);
		}

		//nTroncos = rand() % 7;
		for (nTroncos = 0; nTroncos < 7; nTroncos++) {
			FERROR(WaitForSingleObject(control, INFINITE), WAIT_FAILED, "WaitForSingleObject()\n"); //Reserva posición
			funciones.avanceTroncos(nTroncos);
			if ((*pY > 3 && *pY < 11) && ((*pY) == 10 - nTroncos)) {
				if (dirs[nTroncos]){//Si troncos se mueven a la izquierda
					EnterCriticalSection(&sc);
					//INT InterlockedDecrement(*pX);
					(*pX)--;
					LeaveCriticalSection(&sc);
				}
				else {
					EnterCriticalSection(&sc);
					//INT InterlockedIncrement(*pX);
					(*pX)++;
					LeaveCriticalSection(&sc);
				}
			}
			funciones.pausa();
			FERROR(ReleaseMutex(control), 0, "ReleaseMutex()\n");
		}
	}
}

DWORD WINAPI moverRanas(LPVOID lpParam) {
	int sentido;
	PINT preX, preY, posX, posY;

	//PUNTERO MEMORIA COMPARTIDA A CERO
	preX = (PINT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PINT));
	preY = (PINT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PINT));
	posX = (PINT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PINT));
	posY = (PINT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PINT));
	*preX = (15 + 16 * posicion);
	*preY = 0;

	while (TRUE) {
		//FERROR(WaitForSingleObject(control, INFINITE), WAIT_FAILED, "WaitForSingleObject()\n");
		EnterCriticalSection(&sc);
		*pX = *preX;
		*pY = *preY;
		LeaveCriticalSection(&sc);
		

		if ((*preX) < 0 || (*pX) > 79) {
			(*perdidas)++;
			FERROR(ReleaseMutex(mu[*preX][*preY]), 0, "ReleaseMutex()\n");
			*preY = *preX = -1;
			FERROR(ReleaseMutex(control), 0, "ReleaseMutex()\n");
			break;
		}

		EnterCriticalSection(&sc);
		*pX = *preX;
		*pY = *preY;
		LeaveCriticalSection(&sc);

		FERROR(WaitForSingleObject(control, INFINITE), WAIT_FAILED, "WaitForSingleObject()\n");
		EnterCriticalSection(&sc);
		*preX = *pX;
		*preY = *pY;
		LeaveCriticalSection(&sc);
		if (funciones.puedoSaltar(*preX, *preY, ARRIBA)) sentido = ARRIBA;
		else if (funciones.puedoSaltar(*preX, *preY, IZQUIERDA)) sentido = IZQUIERDA;
		else if (funciones.puedoSaltar(*preX, *preY, DERECHA)) sentido = DERECHA;
		else {
			funciones.pausa();
			continue;
		}

		*posX = *preX;
		*posY = *preY;
		FERROR(WaitForSingleObject(mu[*preX][*preY], INFINITE), WAIT_FAILED, "WaitForSingleObject()\n"); //Reserva posición
		funciones.avanceRanaIni(*preX, *preY);
		funciones.avanceRana(preX, preY, sentido); //Produce el movimiento. Dejan la posición de después
		FERROR(ReleaseMutex(mu[*posX][*posY]), 0, "ReleaseMutex()\n"); //Libera la posición reservada anteriormente

		EnterCriticalSection(&sc);
		*pX = *preX;
		*pY = *preY;
		LeaveCriticalSection(&sc);

		if ((*preX) < 0 || (*preX) > 79) {
			(*perdidas)++;
			FERROR(ReleaseMutex(mu[*preX][*preY]), 0, "ReleaseMutex()\n");
			*preY = *preX = -1;
			FERROR(ReleaseMutex(control), 0, "ReleaseMutex()\n");
			break;
		}

		FERROR(WaitForSingleObject(mu[*preX][*preY], INFINITE), WAIT_FAILED, "WaitForSingleObject()\n"); //Reserva posición a la que va a saltar
		funciones.avanceRanaFin(*preX, *preY);
		if (*preY == 11) {
			(*salvadas)++;
			FERROR(ReleaseMutex(mu[*preX][*preY]), 0, "ReleaseMutex()\n");
			*preY = *preX = -1;
			FERROR(ReleaseMutex(control), 0, "ReleaseMutex()\n");
			break;
		}
		FERROR(ReleaseMutex(mu[*preX][*preY]), 0, "ReleaseMutex()\n"); //Libera posición en la que saltó
		FERROR(ReleaseMutex(control), 0, "ReleaseMutex()\n");
	}
	return 0; //El hilo termina
}

/* ============= Función para tratar argumentos ============= */
void tratarArg(int argc, char* argv[]) {
	int param1, param2;

	if (argc != 3) {
		fprintf(stderr, "%s\n", "USO: ./ranas VELOCIDAD VELOCIDAD_PARTO");
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
	//Funciones BOOL
	FERROR(funciones.ranasDLL = LoadLibrary(TEXT("ranas.dll")), NULL, "LoadLibrary().\n");
	FERROR(funciones.avanceRana = (TIPO_AVANCERANA)GetProcAddress(funciones.ranasDLL, "AvanceRana"), NULL, "GetProcAddress()");
	FERROR(funciones.avanceRanaFin = (TIPO_AVANCERANAFIN)GetProcAddress(funciones.ranasDLL, "AvanceRanaFin"), NULL, "GetProcAddress()");
	FERROR(funciones.avanceRanaIni = (TIPO_AVANCERANAINI)GetProcAddress(funciones.ranasDLL, "AvanceRanaIni"), NULL, "GetProcAddress()");
	FERROR(funciones.avanceTroncos = (TIPO_AVANCETRONCOS)GetProcAddress(funciones.ranasDLL, "AvanceTroncos"), NULL, "GetProcAddress()");
	FERROR(funciones.comprobarEstadisticas = (TIPO_COMPROBARESTADISTICAS)GetProcAddress(funciones.ranasDLL, "ComprobarEstadIsticas"), NULL, "GetProcAddress()");
	FERROR(funciones.finRanas = (TIPO_FINRANAS)GetProcAddress(funciones.ranasDLL, "FinRanas"), NULL, "GetProcAddress()");
	FERROR(funciones.inicioRanas = (TIPO_INICIORANAS)GetProcAddress(funciones.ranasDLL, "InicioRanas"), NULL, "GetProcAddress()");
	FERROR(funciones.partoRanas = (TIPO_PARTORANAS)GetProcAddress(funciones.ranasDLL, "PartoRanas"), NULL, "GetProcAddress()");
	FERROR(funciones.puedoSaltar = (TIPO_PUEDOSALTAR)GetProcAddress(funciones.ranasDLL, "PuedoSaltar"), NULL, "GetProcAddress()");
	//Funciones VOID
	FERROR(funciones.pausa = (TIPO_PAUSA)GetProcAddress(funciones.ranasDLL, "Pausa"), NULL, "GetProcAddress()");
	FERROR(funciones.printMSG = (TIPO_PRINTMSG)GetProcAddress(funciones.ranasDLL, "PrintMsg"), NULL, "GetProcAddress()");

	return 0;
}
