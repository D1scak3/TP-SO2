#define _CRT_SECURE_NO_WARNINGS	//TOU FARTO DESTA PORCARIA


#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#define MAX_PASSAGEIROS 100
#define MAX_AEROPORTOS 20
#define MAX_AVIOES 5
#define TAM 200
#define TAM_BUFFER_CIRCULAR 10		//tamanho buffer circular

//programa controlador
#define ALREADY_CREATED_FLAG TEXT("este_valor_e_usado_para_garantir_que_so_existe_uma_instancia_a_correr_de_cada_vez")

//programa passageiros
#define WAIT_FOR_PASSAGEIRO_TO_CONNECT_EVENT TEXT("WAIT_FOR_PASSAGEIRO_TO_CONNECT_EVENT")
#define WAIT_FOR_PASSAGEIRO_TO_CONNECT_MUTEX TEXT("WAIT_FOR_PASSAGEIRO_TO_CONNECT_MUTEX")
#define WAIT_FOR_PASSAGEIRO_TO_CONNECT_MAP_OF_FILE TEXT("WAIT_FOR_PASSAGEIRO_TO_CONNECT_MAP_OF_FILE")

//programa avi�es
#define MUTEX_PARA_AVIOES_ESCREVEREM_COM_CALMA TEXT("MUTEX_PARA_AVIOES_ESCREVEREM_COM_CALMA")
#define SEMAFORO_PARA_CONTAGEM_DAS_ESCRITAS_PARA_OS_AVIOES TEXT("SEMAFORO_PARA_CONTAGEM_DAS_ESCRITAS_PARA_OS_AVIOES")
#define SEMAFORO_PARA_CONTAGEM_DAS_LEITURAS_PARA_OS_AVIOES TEXT("SEMAFORO_PARA_CONTAGEM_DAS_LEITURAS_PARA_OS_AVIOES")
#define NAME_DO_BUFFER_CIRCULAR_CONHECIDO_POR_TODOS TEXT("NAME_DO_BUFFER_CIRCULAR_CONHECIDO_POR_TODOS")
#define NOME_MAPA_PRIVADO TEXT("NOME_MAPA_PRIVADO")
#define NOME_EVENTO_PRIVADO TEXT("NOME_EVENTO_PRIVADO")



typedef struct {
	TCHAR aeroportoOrigem[TAM], aeroportoDestino[TAM], nome[TAM];
	//possivelmente um nome de um pipe para conectar numa thread
}passageiro, *pPassageiro;
typedef struct {
	TCHAR nome[TAM];
	int x, y;
	boolean listaOcupa��o[MAX_PASSAGEIROS];		//FALSE-> TEM ESPA�O PARA NOVO PASSAGEIRO. TRUE-> J� TEM L� UM PASSAGEIRO
}aeroporto, *pAeroporto;
typedef struct {
	int x, y;
	int capacidade;
	int numPassageirosABordo;
	pPassageiro *listaPassageiros;			//alocar memoria para a capacidade lmao
	TCHAR destino[TAM];

	//velocidade apenas � conhecida pelo avi�o
}aviao, *pAviao;
typedef struct {
	TCHAR nomeEvento[TAM];
	TCHAR nomePrivateMap[TAM];
}celula;
typedef struct {
	int proxEscrita;
	int proxLeitura;
	int maxPos;
	celula buffer[TAM_BUFFER_CIRCULAR];
}buffer_circular;


void escreveNoRegistry(HKEY chave, TCHAR par_nome[], TCHAR par_valor[]);
void closeProgram(TCHAR mesage[]);
int obtemIntDoregistry(HKEY chaveRegistry, TCHAR par_nome[]);
void criarAeroporto();
void criaConeccaoComAvioes();
void verificaUnicidade();
void ObtemValoresDoRegistry();

//thread methods
DWORD WINAPI atendePassageiros(LPVOID lpParam);			//n�o envia nada para esta thread
DWORD WINAPI trataBufferAvioes(LPVOID lpParam);			//n�o envia nada para esta thread
DWORD WINAPI atendeAviao(LPVOID lpParam);			//n�o envia nada para esta thread


//geral do programa
HKEY chaveRegistry = NULL;
HANDLE verificador;


//GENERAL
boolean turnOffFlag;



int max_aeroportos;		//define n�mero m�ximo de aeroportos
int max_avioes;		//define n�mero m�ximo de avi�es

//cenas do mapa em si
pAeroporto listaAeroportos;	//lista que guarda os aeroportos
int numAeroportos;			//n�mero de aeroportos que existem (�ltimo aeroporto criado -> listaAeroportos[numAeroportos - 1])

//Passageiros
pPassageiro listaPassageiros[MAX_PASSAGEIROS];
int numPassageiros;
HANDLE atendePassageirosEvent, atendePassageirosMutex;
TCHAR* mapViewOfFileEventoPassageiros;

//Avi�es
int lastEscrito;
int lastLido;
buffer_circular* bufferCircular;
HANDLE hMutexAvioes;
HANDLE hSemafotoEscrita;
HANDLE hSemafotoLeitura;
HANDLE hBufferCircular;

int _tmain(int argc, TCHAR* argv[]) {
	
	

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	//verifica se j� existe uma instancia a correr ou n�o
	verificaUnicidade();

	ObtemValoresDoRegistry();
	


_tprintf(TEXT("\nnum max de avi�es: %d"), max_avioes);
_tprintf(TEXT("\nnum max de aeroportos: %d"), max_aeroportos);
_tprintf(TEXT("\n"));

listaAeroportos = malloc(max_aeroportos * sizeof(aeroporto));
numAeroportos = 0;

//cria threads extra----------------------------------------------------------------

//CRIA THREAD EVENTO E TUDO PARA CONE��O COM OS AVI�ES
criaConeccaoComAvioes();


//loop para criar aeroportos e desligar

do {
	TCHAR input[TAM];

	_tprintf(TEXT("\n1- criar Aeroporto"));
	_tprintf(TEXT("\n'fim' para sair"));
	_tprintf(TEXT("\n"));

	_fgetts(input, TAM - 1, stdin);
	input[_tcslen(input) - 1] = '\0';

	if (0 == _tcscmp(TEXT("fim"), input)) {
		break;
	}
	else if (0 == _tcscmp(TEXT("1"), input)) {
		criarAeroporto();
	}



} while (1);

CloseHandle(verificador);	//elimina a handle para que permita que o controlador possa ser ligado multiplas vezes
return 0;
}


//verifica��o inicial do registry
void escreveNoRegistry(HANDLE chave, TCHAR par_nome[], TCHAR par_valor[]) {
	//adicionar par nome-valor
	if (RegSetValueEx(chave, par_nome, 0, REG_SZ, (LPBYTE)par_valor, sizeof(TCHAR) * (_tcslen(par_valor) + 1)) == ERROR_SUCCESS)
		_tprintf(TEXT("\nPar chave-valor com nome <%s> e valor <%s> escrita com sucesso no registry"), par_nome, par_valor);
	else
	{
		TCHAR mesage[TAM];
		_stprintf_s(mesage, TAM, TEXT("\nFalha a escrever no resgistry chave-valor com nome <%s> e valor <%s>"), par_nome, par_valor);
		closeProgram(mesage);
	}
}
int obtemIntDoregistry(HKEY chaveRegistry, TCHAR par_nome[]) {
	TCHAR value_obtained[TAM];
	DWORD size = TAM;
	//consultar par nome-valor
	if (RegQueryValueEx(chaveRegistry, par_nome, NULL, NULL, (LPBYTE)value_obtained, &size) == ERROR_SUCCESS)
		return _tstoi(value_obtained);
		
	return -1;
}

//UI Methods
void criarAeroporto() {

	if (numAeroportos >= max_aeroportos)
	{
		_tprintf(TEXT("\nN�o � possivel criar mais aeroportos!\n"));
		return;
	}
	TCHAR nome[TAM];
	boolean flag =	FALSE;

	//obtem nome
	while (!flag) {
		flag = TRUE;
		_tprintf(TEXT("indique nome do aeroporto:\t"));
		_tscanf_s(TEXT("%s"), &nome, TAM);

		for (int i = 0; i < numAeroportos; i++) {
			if (0 == _tcscmp(nome, listaAeroportos[i].nome)){
				_tprintf(TEXT("\nEste nome <%s> j� existe, coloque outro"), nome);
				flag = FALSE;
			}
		}
	}

	//obtem coordenadas
	flag = FALSE;
	int x, y;
	while (!flag) {
		flag = TRUE;
		_tprintf(TEXT("\nindique posi��o X (0 - 999):\t"));
		_tscanf_s(TEXT("%d"), &x, TAM);

		_tprintf(TEXT("\nindique posi��o Y (0 - 999):\t"));
		_tscanf_s(TEXT("%d"), &y, TAM);

		if (x > 999 || x < 0){
			_tprintf(TEXT("\nValor de X inv�lido"));
			flag = FALSE;
		}
		else if (y > 999 || y < 0) {
			_tprintf(TEXT("\nValor de Y inv�lido"));
			flag = FALSE;
		}
		else
			for (int i = 0; i < numAeroportos; i++) {
				long distancia = sqrt(pow((listaAeroportos[i].x - x), 2) + pow((listaAeroportos[i].y - y), 2));	//distancia entre dois pontos

				if (distancia > 10) {
					_tprintf(TEXT("\nCoordenadas do aeroporto a menos de 10 casas do aeroporto mais pr�ximo"));
					flag = FALSE;
				}
			}
	}

	//coloca os valores na posi��o devida
	wcscpy_s(listaAeroportos[numAeroportos].nome,TAM, nome);
	listaAeroportos[numAeroportos].x = x;
	listaAeroportos[numAeroportos].y = y;
	
	//coloca a lista a indicar que os lugares est�o todos livres
	for (int i = 0; i < MAX_PASSAGEIROS; i++)
		listaAeroportos[numAeroportos].listaOcupa��o[i] = FALSE;
	
	//Aumenta o counter
	numAeroportos++;									


}

//Generalist Error Methods
void closeProgram(TCHAR mesage[]) {
	if (chaveRegistry != NULL && RegCloseKey(chaveRegistry) == ERROR_SUCCESS)
		_tprintf(TEXT("\nErro cr�tico! Mensagem de erro:\t%s\nA sair..."), mesage);
	else
		_tprintf(TEXT("\nErro a fechar chave de registo!\n\nA DESLIGAR NA MESMA!"));

	CloseHandle(verificador);	//elimina a handle para que permita que o controlador possa ser ligado multiplas vezes
			
	exit(0);
}


//Thread Methods
DWORD WINAPI atendePassageiros(LPVOID lpParam) {
	//MAPEA PASSAGEIROS NA MEM�RIA
	HANDLE hFileMapping = CreateFileMappingA(
		INVALID_HANDLE_VALUE,		//usado para quando n�o h� um ficheiro a abrir, � s� na ram
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(passageiro),						//coloca l� os dados do passageiro
		WAIT_FOR_PASSAGEIRO_TO_CONNECT_MAP_OF_FILE
	);

	if (hFileMapping == NULL) {
		_tprintf(TEXT("\nErro a mapear ficheiro < %s > em mem�ria-> %d"), WAIT_FOR_PASSAGEIRO_TO_CONNECT_MAP_OF_FILE, GetLastError());
		return 1;
	}

	mapViewOfFileEventoPassageiros = (passageiro*)MapViewOfFile(
		hFileMapping,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		0);

	if (mapViewOfFileEventoPassageiros == NULL)
	{
		_tprintf(TEXT("\nErro a criar vista de mapa < %s >: %d"), WAIT_FOR_PASSAGEIRO_TO_CONNECT_MAP_OF_FILE, GetLastError());
		return 1;
	}
	//MUTEX
	atendePassageirosMutex = CreateMutex(NULL,
		FALSE,
		WAIT_FOR_PASSAGEIRO_TO_CONNECT_MUTEX);

	if (atendePassageirosMutex == NULL) {
		_tprintf(TEXT("Erro a criar mutex < %S >: %d"), WAIT_FOR_PASSAGEIRO_TO_CONNECT_MUTEX, GetLastError());
		return 1;
	}
	//EVENTO
	atendePassageirosEvent = CreateEvent(
		NULL,
		TRUE,
		FALSE,
		WAIT_FOR_PASSAGEIRO_TO_CONNECT_MUTEX);

	if (atendePassageirosEvent == NULL)
	{
		_tprintf(TEXT("Erro a criar evento < %S >: %d"), WAIT_FOR_PASSAGEIRO_TO_CONNECT_EVENT, GetLastError());
		return 1;
	}

	while(!turnOffFlag) {			//isto � uma maneira horrivel para fazer com que o progranma desligue
		WaitForSingleObject(atendePassageirosEvent, INFINITE);
		
		WaitForSingleObject(atendePassageirosMutex, INFINITE);
			
		pPassageiro newPassageiro = mapViewOfFileEventoPassageiros;


		ReleaseMutex(atendePassageirosMutex);
	
		ResetEvent(atendePassageirosEvent);
	}


}

DWORD WINAPI trataBufferAvioes(LPVOID lpParam) {
	//espera ter lugar no semaforo para escrita
	//coloca dados iniciais no buffer circular:
	
	int counter = 0;
	
	while (!turnOffFlag) {
		
		//cria evento privado
		//cria mapa privado
		//lanca thread com mapa privado e evento privado
		//escreve no mapa publico

		WaitForSingleObject(hSemafotoEscrita, INFINITE); //espera ter lugar no semaforo para escrita
		
		TCHAR nome_evento[TAM - 6], nome_mapa[5];
		TCHAR num[TAM];
		_itot(counter, num, 10);	//into to tchar

		//obtem nome completo evento
		wcscpy_s(nome_evento, TAM - 6, NOME_EVENTO_PRIVADO);
		wcscat(nome_evento, num);
		//obtem nome completo mapa
		wcscpy_s(nome_mapa, TAM - 6, NOME_EVENTO_PRIVADO);
		wcscat(nome_mapa, num);	
		counter++;	//aumenta o counter
		
					//cria o evento
		HANDLE newEvento = CreateEvent(
			NULL,
			TRUE,
			FALSE,
			nome_evento);

		if (newEvento == NULL)
		{
			_tprintf(TEXT("Erro a criar evento: %d"), GetLastError());
			return 1;
		}

		//cria o mapa
		HANDLE newMapa =  CreateFileMapping(
			INVALID_HANDLE_VALUE,
			NULL,
			PAGE_READWRITE,
			0,
			sizeof(aviao), //tamanho da memoria partilhada
			nome_mapa);//nome do filemapping. nome que vai ser usado para partilha entre processos

		if (newMapa == NULL) {
			_tprintf(TEXT("Erro no CreateFileMapping\n"));
			return -1;
		}


		//****************************************************************************
		//****************************************************************************
				//LAN�A THREAD COM EVENTO E HANDLE DE MAPA
		//****************************************************************************
		//****************************************************************************


		celula cell;
		wcscpy(cell.nomeEvento, nome_evento);
		wcscpy(cell.nomePrivateMap, nome_mapa);

		//escreve no mapa publico
		WaitForSingleObject(hMutexAvioes, INFINITE);
		//coloca l� os dados de cone��o
		CopyMemory(&bufferCircular->buffer[bufferCircular->proxEscrita] , &cell, sizeof(celula));
		//aumenta posi��o para pr�xima escrita
		if (bufferCircular->proxEscrita == bufferCircular->maxPos)
			bufferCircular->proxEscrita = 0;
		else
			bufferCircular->proxEscrita++;

		ReleaseMutex(hMutexAvioes);

		ReleaseSemaphore(hSemafotoLeitura, 1, NULL);	//adiciona 1 posi��o para leitura
	}

}

void criaConeccaoComAvioes() {

	hMutexAvioes = CreateMutex(NULL, FALSE, MUTEX_PARA_AVIOES_ESCREVEREM_COM_CALMA);
	hSemafotoEscrita = CreateSemaphore(NULL, TAM_BUFFER_CIRCULAR, TAM_BUFFER_CIRCULAR, SEMAFORO_PARA_CONTAGEM_DAS_ESCRITAS_PARA_OS_AVIOES);
	hSemafotoLeitura = CreateSemaphore(NULL, 0, TAM_BUFFER_CIRCULAR, SEMAFORO_PARA_CONTAGEM_DAS_LEITURAS_PARA_OS_AVIOES);

	if (hMutexAvioes == NULL || hSemafotoEscrita == NULL || hSemafotoLeitura == NULL) {
		_tprintf(TEXT("Erro no CreateSemaphore ou no CreateMutex\n"));
		return -1;
	}

	hBufferCircular = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, NAME_DO_BUFFER_CIRCULAR_CONHECIDO_POR_TODOS);
	if (hBufferCircular == NULL) {

		//criamos o bloco de memoria partilhada
		hBufferCircular = CreateFileMapping(
			INVALID_HANDLE_VALUE,
			NULL,
			PAGE_READWRITE,
			0,
			sizeof(buffer_circular), //tamanho da memoria partilhada
			NAME_DO_BUFFER_CIRCULAR_CONHECIDO_POR_TODOS);//nome do filemapping. nome que vai ser usado para partilha entre processos

		if (hBufferCircular == NULL) {
			_tprintf(TEXT("Erro no CreateFileMapping\n"));
			return -1;
		}
	}

	bufferCircular = (buffer_circular*)MapViewOfFile(hBufferCircular, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	bufferCircular->proxEscrita = 0;
	bufferCircular->proxLeitura = 0;
	bufferCircular->maxPos = TAM_BUFFER_CIRCULAR;
	//lan�a  a thread
	HANDLE  threadConectaAvioes = CreateThread(NULL, 0, trataBufferAvioes, NULL, 0, NULL);
	if (threadConectaAvioes != NULL) {
		_tprintf(TEXT("Erro a criar thread para cone��o com os avi�es-> %d"), GetLastError());
		exit(-99);
	}

}

void verificaUnicidade() {

	verificador = CreateFileMappingA(
		INVALID_HANDLE_VALUE,		//usado para quando n�o h� um ficheiro a abrir, � s� na ram
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(TCHAR),
		ALREADY_CREATED_FLAG
	);
	if (verificador == NULL) {
		_tprintf(TEXT("N�o foi possivel verificar se existia outra instancia a correr\nA desligar"));
		return 1;
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS) {
		_tprintf(TEXT("J� se encontra ligada uma instancia do controlador\nA desligar"));
		return 1;
	}

}

void ObtemValoresDoRegistry() {

	TCHAR chave_nome[TAM], par_nome[TAM], par_valor[TAM];
	TCHAR chave_avioes[] = TEXT("Software\\SO2\\TPMDMF\\AVIOES\\");
	TCHAR chave_aeroportos[] = TEXT("Software\\SO2\\TPMDMF\\AEROPORTOS\\");


	//Obtem valores para avi�es
	if (RegCreateKeyEx(HKEY_CURRENT_USER, chave_avioes, 0, NULL, REG_OPTION_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &chaveRegistry, NULL) != ERROR_SUCCESS)
	{
		_tprintf(TEXT("\nErro critico a ler chave de avi�es!\nA desligar programa"));
		exit(-1);
	}
	//tenta obter
	wcscpy_s(par_nome, TAM, TEXT("MAXAVIOES"));
	max_avioes = obtemIntDoregistry(chaveRegistry, par_nome);
	if (max_avioes == -1)	//caso n�o exista para obter, escreve
	{
		wcscpy_s(par_nome, TAM, TEXT("MAXAVIOES"));
		_stprintf_s(par_valor, TAM, TEXT("%d"), MAX_AVIOES);
		escreveNoRegistry(chaveRegistry, par_nome, par_valor);
		max_avioes = MAX_AVIOES;
	}


	//fecha a handle de avi�es
	CloseHandle(chaveRegistry);

	if (RegCreateKeyEx(HKEY_CURRENT_USER, chave_aeroportos, 0, NULL, REG_OPTION_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &chaveRegistry, NULL) != ERROR_SUCCESS) {
		_tprintf(TEXT("\nErro critico a ler a chave do controlador2!\nA desligar o programa"));
		exit(-1);
	}
	//tenta obter
	wcscpy_s(par_nome, TAM, TEXT("MAXAEROPORTOS"));
	max_aeroportos = obtemIntDoregistry(chaveRegistry, par_nome);
	if (max_aeroportos == -1)//caso n�o exista para obter, escreve 
	{
		wcscpy_s(par_nome, TAM, TEXT("MAXAEROPORTOS"));
		_stprintf_s(par_valor, TAM, TEXT("%d"), MAX_AEROPORTOS);
		escreveNoRegistry(chaveRegistry, par_nome, par_valor);
		max_aeroportos = MAX_AEROPORTOS;
	}

	//fecha a handle de aeroportos
	CloseHandle(chaveRegistry);

}
