//////////////////////////////////////////////////////////////////
//	TP1 - Sistemas electrónicos									//
//	Grupo 1:												  	//
//  Clara Ruiz de las Heras, Mario Medrano Paredes,				//
//  Miguel Barrigón Gómez, Víctor Sánchez Valencia			    //
//////////////////////////////////////////////////////////////////

// Funciones para leer la posicion GPS
void lee_GPS();
int GPS_lee_latlon(char *lat, int lat_len, char *lon, int lon_len);
int GPS_lee_latlon_micro(int *lat_micro, int *lon_micro);