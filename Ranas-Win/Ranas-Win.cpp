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

#define HOR_MIN 0
#define HOR_MAX 79
#define VER_MIN 0
#define VER_MAX 12 //Orilla
#define TIEMPO 30000

#define FERROR(ReturnValue,ErrorValue,ErrorMsg)											\
    do{																					\
        if((ReturnValue) == (ErrorValue)){												\
            fprintf(stderr, "\n[%d:%s] FERROR: %s", __LINE__, __FUNCTION__,ErrorMsg);	\
        }																				\
    }while(0)

struct funcionesDLL //Guarda funciones .dll en un struct
{
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

/* ======================================= FUNCIONES ======================================= */
void tratarArg(int argc, char* argv[]); //Tratamiento argumentos
int cargarRanas(); //Carga de biblioteca .dll
void criar(int pos); //Función a la que llama cada hilo madre
DWORD WINAPI moverRanas(LPVOID lpParam); //Función que realiza cada ranita


/* ======================================= VAR GLOBLALES ======================================= */
PLONG nacidas, salvadas, perdidas;
int posicion; //Se utiliza para pasarle "pos" a los movXY de moverRanas desde f_criar
int lTroncos[] = { 10,10,10,10,10,10,10 };
int lAguas[] = { 1,1,1,1,1,1,1 };
int dirs[] = { 1,0,1,0,1,0,1 };
int noTerminado = 1;
int ranasTroncos[12][80];

//MUTEXES
HANDLE mu[80][12], control;
CRITICAL_SECTION sc0;

/* ======================================= MAIN ======================================= */

int main(int argc, char* argv[])
{
	system("mode con:cols=80 lines=25"); //FIJA AUTOMÁTICAMENTE A 80x25
	setlocale(LC_ALL, "");
	FERROR(SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS), FALSE, "SetPriorityClass"); //PRIORIDAD DEL PROCESO NORMAL --> IDLE

	int velocidad, parto;
	int i, j;

	tratarArg(argc, argv);
	velocidad = atoi(argv[1]);
	parto = atoi(argv[2]);

	FERROR(cargarRanas(), -1, "cargarRanas()\n");

	/***********************************************************************************************************************************
	************************************************************RECURSOS****************************************************************
	***********************************************************************************************************************************/
	//VARIABLES
	nacidas = (PLONG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PLONG));
	salvadas = (PLONG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PLONG));
	perdidas = (PLONG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PLONG));
	*nacidas = *salvadas = *perdidas = 0;

	for (i = VER_MIN; i < VER_MAX; i++)
		for (j = HOR_MIN; j < HOR_MAX; j++)
			ranasTroncos[i][j] = 0;

	//HANDLES (Mutex)
	for (i = HOR_MIN; i <= HOR_MAX; i++)
		for (j = VER_MIN; j < VER_MAX; j++)
			FERROR(mu[i][j] = (HANDLE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HANDLE)), NULL, "HeapAlloc()\n");
	//CARGAR HANDLE mu CON MUTEX
	for (i = HOR_MIN; i <= HOR_MAX; i++)
		for (j = VER_MIN; j < VER_MAX; j++)
			FERROR(mu[i][j] = CreateMutex(NULL, FALSE, NULL), NULL, "CreateMutex()\n");

	FERROR(control = (HANDLE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HANDLE)), NULL, "HeapAlloc()\n");
	FERROR(control = CreateMutex(NULL, FALSE, NULL), NULL, "CreateMutex()\n");
	
	//HANDLE (Seccion Critica)
	InitializeCriticalSection(&sc0);
	
	/***********************************************************************************************************************************
	*******************************************************INICIO DEL PROGRAMA**********************************************************
	***********************************************************************************************************************************/
	FERROR(funciones.inicioRanas(velocidad, lTroncos, lAguas, dirs, parto, criar), FALSE, "inicioRanas()\n");

	Sleep(TIEMPO);

	//CERRAMOS RECURSOS
	noTerminado = 0;
	funciones.pausa();
	funciones.finRanas();

	for (i = HOR_MIN; i <= HOR_MAX; i++)
		for (j = VER_MIN; j < VER_MAX; j++)
			CloseHandle(mu[i][j]);

	CloseHandle(control);

	DeleteCriticalSection(&sc0);

	funciones.comprobarEstadisticas(*nacidas, *salvadas, *perdidas);

	return 0;
}

void criar(int pos) {
	int mov = 0;
	EnterCriticalSection(&sc0); //Para control de Posicion
	while (noTerminado) {
		WaitForSingleObject(mu[(15 + 16 * pos)][0], INFINITE);
		if (funciones.partoRanas(pos)) {
			(*nacidas)++;
			posicion = pos;
			CreateThread(NULL, 0, moverRanas, 0, 0, NULL);
		}
		ReleaseMutex(mu[(15 + 16 * pos)][0]);

		for (int nTroncos = 0; nTroncos < 7; nTroncos++) {
			WaitForSingleObject(control, INFINITE); //Reserva posición
			funciones.avanceTroncos(nTroncos);
			if (dirs[nTroncos])//Si troncos se mueven a la izquierda
				mov = -1;
			else
				mov = 1;
			funciones.pausa();
			for (int i = HOR_MIN; i < HOR_MAX; i++) {
				if (ranasTroncos[(10 - nTroncos)][i] < 0 || ranasTroncos[(10 - nTroncos)][i] > 79) ranasTroncos[(10 - nTroncos)][i] = 0; //Si se desborda mete un 0
				if (ranasTroncos[(10 - nTroncos)][i] != 0) {
					WaitForSingleObject(mu[(10 - nTroncos)][i], INFINITE);
					ranasTroncos[(10 - nTroncos)][i] = ranasTroncos[(10 - nTroncos)][i] + mov; //Mete en el valor actual de la ranita, la posición nueva que va a tener
					ReleaseMutex(mu[(10 - nTroncos)][i]);
				}
			}
			ReleaseMutex(control);
		}
	}
}

DWORD WINAPI moverRanas(LPVOID lpParam) {
	int sentido;
	int posX, posY, preX, preY, valor, tempX;
	
	valor = (15 + 16 * posicion); //Control posicion
	LeaveCriticalSection(&sc0);

	//Valores iniciales ranita
	WaitForSingleObject(mu[(15 + 16 * posicion)][0], INFINITE);
	tempX = preX = posX = valor;
	preY = posY = 0;
	ranasTroncos[preY][preX] = preX;
	ReleaseMutex(mu[(15 + 16 * posicion)][0]);

	while (noTerminado) {
		WaitForSingleObject(control, INFINITE); //Cada ranita posee el mutex hasta que hace avance efectivo
		//ACTUALIZA POSICIÓN
		if (ranasTroncos[posY][posX] == 0) {
			funciones.pausa();
			ReleaseMutex(control);
			ExitThread(777);
			break;
		}

		WaitForSingleObject(mu[posX][posY], INFINITE);
		tempX = posX;
		posX = ranasTroncos[posY][posX]; //Ranita actualiza posicion por si tronco la ha movido
		ranasTroncos[preY][preX] = 0; //Mete un 0 en la posicion anterior
		ReleaseMutex(mu[tempX][posY]);

		//COMPROBACION SI SALE DE LA PANTALLA
		if ((tempX) < 0 || (tempX) > 79 || (posX) < 0 || (posX) > 79 || (preX) < 0 || (preX) > 79) {
			(*perdidas)++;
			ReleaseMutex(control);
			break;
		}

		if (funciones.puedoSaltar(posX, posY, ARRIBA)) sentido = ARRIBA;
		else if (funciones.puedoSaltar(posX, posY, IZQUIERDA)) sentido = IZQUIERDA;
		else if (funciones.puedoSaltar(posX, posY, DERECHA)) sentido = DERECHA;
		else {
			funciones.pausa();
			ReleaseMutex(control);
			continue;
		}

		preX = posX;
		preY = posY;
		WaitForSingleObject(mu[posX][posY], INFINITE);
		if (funciones.avanceRanaIni(posX, posY) == FALSE) {
			ReleaseMutex(mu[posX][posY]);
			ReleaseMutex(control);
			ExitThread(777);
			break;
		}
		funciones.avanceRana(&posX, &posY, sentido);
		ReleaseMutex(mu[preX][preY]); //Libera el mutex anterior

		funciones.pausa();

		//ACTUALIZA POSICIÓN
		WaitForSingleObject(mu[posX][posY], INFINITE);
		ranasTroncos[posY][posX] = posX; //Mete nueva posicion en el array
		ranasTroncos[preY][preX] = 0; // Pone a 0 la antigua
		ReleaseMutex(mu[posX][posY]);

		//COMPROBACION SI SALE DE LA PANTALLA
		if ((tempX) < 0 || (tempX) > 78 || (posX) < 0 || (posX) > 78 || (preX) < 0 || (preX) > 78) {
			(*perdidas)++;
			ReleaseMutex(control);
			break;
		}

		WaitForSingleObject(mu[posX][posY], INFINITE);
		funciones.avanceRanaFin(posX, posY);
		if (posY == 11) { //Si llega a orilla
			(*salvadas)++;
			ReleaseMutex(control);
			break;
		}
		ReleaseMutex(mu[posX][posY]);
		ReleaseMutex(control);
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
