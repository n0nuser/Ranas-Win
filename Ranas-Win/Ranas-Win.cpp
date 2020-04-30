#include <stdio.h> 
#include <stdlib.h>
#include <Windows.h>
#include <iostream>
#include "ranas.h"

#define HOR_MIN 0
#define HOR_MAX 79
#define VER_MIN 0
#define VER_MAX 11
#define MSG_ERROR "GetProcAddress FERROR."


#define FERROR(ReturnValue,ErrorValue,ErrorMsg)							\
    do{																					\
        if((ReturnValue) == (ErrorValue)){												\
            fprintf(stderr, "\n[%d:%s] FERROR: %s", __LINE__, __FUNCTION__,ErrorMsg);	\
            exit(EXIT_FAILURE);															\
        }																				\
    }while(0)

typedef BOOL (*TIPO_INICIORANAS)(int,int *,int *,int *,int,TIPO_CRIAR); 
typedef void (*TIPO_CRIAR)(int);
//HMODULE WINAPI RanasLibrary;

typedef HANDLE* RIO;

typedef struct datosRanas {
	int numRana;
	RIO rio[HOR_MAX];
}datosRanas, * pDatosRanas;


//(Y = 0 y 3) == antes de cruzar el río.
//(Y = 11)    == orilla

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
} funciones;

//FUNCIONES
void tratarArg(int argc, char* argv[]);
int cargarRanas();

//MAIN
int main(int argc, char* argv[])
{
	system("mode con:cols=80 lines=25"); //FIJA AUTOMÁTICAMENTE A 80x25

	/*HANDLE hilo1, hilo2;
	CHAR char1, char2;
	DWORD id1, id2;
	INT i, reps;
	*/
	int velocidad, parto;
	int lTroncos[] = {1,1,1,1,1,1,1};
	int lAguas[] = {1,1,1,1,1,1,1};
	int dirs[] = {1,1,1,1,1,1,1};

	//TRATAMIENTO ARGUMENTOS
	tratarArg(argc, argv);
	velocidad = atoi(argv[1]);
	parto = atoi(argv[2]);


	if (cargarRanas() == -1) {
		PERROR("FERROR al cargar la biblioteca ranas.");
		exit(2);
	}
	
	/*
	RanasLibrary = LoadLibrary(TEXT("ranas.dll"));
	if (RanasLibrary == NULL) {
		fprintf(stderr, "La biblioteca no se ha podido cargar correctamente. Abortando... %d", GetLastError());
		return 1;
	}
	*/

	TIPO_CRIAR f_criar = NULL;

	funciones.inicioRanas(velocidad, lTroncos, lAguas, dirs, parto, f_criar);
	
	Sleep(4000); // Se debe esperar 30 segundos para finalizar el programa
	
	return 0;
}

// Funcion f_criar
void f_criar (int pos) {

	pDatosRanas Datos = (pDatosRanas)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(datosRanas));
	Datos->numRana = pos;
	pos = 0;
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
	FERROR(funciones.puedoSaltar = (BOOL(*)(int, int, int)) GetProcAddress(funciones.ranasDLL, "PuedoSaltar"), NULL, "Error: ");
	//Los void no los cargamos
	return 0;
}
