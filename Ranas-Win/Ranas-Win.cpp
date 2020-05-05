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
long nacidas = 0, salvadas = 0, perdidas = 0;
int *posX, *posY, nMadre = 0;

//MUTEXES
HANDLE ranasMutex;

//MAIN Comentario
int main(int argc, char* argv[])
{
	system("mode con:cols=80 lines=25"); //FIJA AUTOMÁTICAMENTE A 80x25
	/*
	//PRIORIDAD DEL PROCESO NORMAL --> IDLE
	if (!SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS)) {
		PERROR("SetPriorityClass");
		return 1;
	}
	*/
	/*
	HANDLE hilo1, hilo2;
	CHAR char1, char2;
	DWORD id1, id2;
	INT i, reps;
	*/
	int velocidad, parto;
	int lTroncos[] = { 4,5,4,5,4,5,4 };
	int lAguas[] = { 5,4,3,5,3,4,5 };
	int dirs[] = { 1,0,1,0,1,0,1 };
	int i;

	//TRATAMIENTO ARGUMENTOS
	tratarArg(argc, argv);
	velocidad = atoi(argv[1]);
	parto = atoi(argv[2]);

	posX = (PINT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(posX));
	posY = (PINT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(posY));

	if (cargarRanas() == -1) {
		PERROR("ERROR al cargar la biblioteca ranas.");
		exit(2);
	}

	FERROR(ranasMutex = CreateMutex(NULL, FALSE, NULL),NULL, "CreateMutex() ERROR.\n");

	funciones.inicioRanas(velocidad, lTroncos, lAguas, dirs, parto, f_criar);
	
	Sleep(30000); // Se debe esperar 30 segundos para finalizar el programa

	funciones.finRanas();

	//Acabar con hilos de ranas
	WaitForMultipleObjects(THREAD_MAX, hilos, TRUE, INFINITE);
	for (i = 0; i < THREAD_MAX; i++) CloseHandle(hilos[i]);
	CloseHandle(ranasMutex);

	funciones.comprobarEstadisticas(nacidas, salvadas, perdidas);
	
	return 0;
}

// Funcion f_criar
void f_criar (int pos) { //Llamará a la función PartoRanas, actualiza las estadísticas y crea un nuevo hilo para mover a la recién nacida.
	//pos = 0;
	if (funciones.partoRanas(pos)) {
		nacidas++;
		*posX = (15 + (16 * pos));
		*posY = 0;
		FERROR(CreateThread(NULL, 0, moverRanas, 0, 0, NULL), NULL, "THREAD_CREATION ERROR");
	} else {
		Sleep(5000);
		printf("Ha ocurrido un error en el parto de las ranas.\n");
		exit(777);
	}
}


//CAMBIAR TODO POR LO DE BATRACIOS (354 ~)
DWORD WINAPI moverRanas(LPVOID lpParam) {
	int sentido = ARRIBA;
	DWORD dwWaitResult;
	for(;;) {
		dwWaitResult = WaitForSingleObject(ranasMutex, INFINITE);

		switch (dwWaitResult){
		case WAIT_OBJECT_0:
			__try {
				//RESERVA MEMORIA COMPARTIDA
				if ((*posX) < 0 || (*posX) > 79) {
					//CERRAR MEMORIA COMPARTIDA
					perdidas++;
					(*posY) = -1;
					(*posX) = -1;
				}

				if (funciones.puedoSaltar(*posX, *posY, ARRIBA)) sentido = ARRIBA;
				else if (funciones.puedoSaltar(*posX, *posY, DERECHA)) sentido = DERECHA;
				else if (funciones.puedoSaltar(*posX, *posY, IZQUIERDA)) sentido = IZQUIERDA;
				else {
					//CERRAR MEMORIA COMPARTIDA
					funciones.pausa();
					printf("No puedo saltar");
					//continue;
				}

				//REVISAR CONDICIONES AVANCE RANA
				funciones.avanceRanaIni(*posX, *posY);
				funciones.avanceRana(posX, posY, sentido); //Produce el movimiento. Dejan la posición de después
				funciones.avanceRanaFin(*posX, *posY);
				
				if (*posY == 11) {
					salvadas++;
					//(*posX) = -1;
					//(*posY) = -1;
					break;
				}
			}
			__finally {
				FERROR(ReleaseMutex(ranasMutex), FALSE, "ReleaseMutex() ERROR.\n");
				salvadas++;
				//(*posX) = -1;
				//(*posY) = -1;
			}

			/*if ((*posY) == 11) {
				//CERRAR MEMORIA COMPARTIDA
				salvadas++;
				(*posX) = -1;
				(*posY) = -1;
				break; //NO QUITARLO!!
			}*/
			//CERRAR MEMORIA COMPARTIDA
		case WAIT_ABANDONED:
			return FALSE;
		}
	}
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