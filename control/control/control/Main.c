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

//ESTADOS PARA OS AVIOES
#define ESTADO_CONECAO_INCIAL 0
#define ESTADO_ESPERA 1
#define ESTADO_VOO 2


//programa controlador
#define ALREADY_CREATED_FLAG TEXT("este_valor_e_usado_para_garantir_que_so_existe_uma_instancia_a_correr_de_cada_vez")

//programa passageiros
#define WAIT_FOR_PASSAGEIRO_TO_CONNECT_EVENT TEXT("WAIT_FOR_PASSAGEIRO_TO_CONNECT_EVENT")
#define WAIT_FOR_PASSAGEIRO_TO_CONNECT_MUTEX TEXT("WAIT_FOR_PASSAGEIRO_TO_CONNECT_MUTEX")
#define WAIT_FOR_PASSAGEIRO_TO_CONNECT_MAP_OF_FILE TEXT("WAIT_FOR_PASSAGEIRO_TO_CONNECT_MAP_OF_FILE")

//programa aviões
#define SEMAFORO_PARA_LIMITE_DE_CONECOES_AVIOES TEXT("SEMAFORO_PARA_LIMITE_DE_CONECOES_AVIOES")
//Aviao2Control
#define BUFFER_AVIAO2CONTROL TEXT("EVENTO_AVIAO2CONTROL")
#define MUTEX_AVIAO2CONTROL TEXT("MUTEX_AVIAO2CONTROL")
#define SEMAFORO_ESCRITA_AVIAO2CONTROL TEXT("SEMAFORO_ESCRITA_AVIAO2CONTROL")
#define SEMAFORO_LEITURA_AVIAO2CONTROL TEXT("SEMAFORO_LEITURA_AVIAO2CONTROL")
//Control2Aviao
#define BUFFER_CONTROL2AVIAO TEXT("BUFFER_CONTROL2AVIAO")
#define MUTEX_CONTROL2AVIAO TEXT("MUTEX_CONTROL2AVIAO")
#define SEMAFORO_ESCRITA_CONTROL2AVIAO TEXT("SEMAFORO_ESCRITA_CONTROL2AVIAO")
#define SEMAFORO_LEITURA_CONTROL2AVIAO TEXT("SEMAFORO_LEITURA_CONTROL2AVIAO")

//aceder a variaveis locais
#define MUTEX_ACEDER_LISTA_AEROPORTOS TEXT("MUTEX_ACEDER_LISTA_AEROPORTOS")
#define MUTEX_ACEDER_LISTA_AVIOES TEXT("MUTEX_ACEDER_LISTA_AVIOES")
#define MUTEX_ACEDER_LISTA_PASSAGEIROS TEXT("MUTEX_ACEDER_LISTA_PASSAGEIROS")


typedef struct {
	int idAviao;
	TCHAR aeroportoOrigem[TAM], aeroportoDestino[TAM], nome[TAM];
	//possivelmente um nome de um pipe para conectar numa thread
}passageiro, *pPassageiro;
typedef struct {
	TCHAR nome[TAM];
	int x, y;
}aeroporto, *pAeroporto;

typedef struct {
	int id;
	int startX, startY, destX, destY, posX, posY;
	int capacidade;
	TCHAR destino[TAM];
	int velocidade;
	int estado; //estado -> ESTADO_CONECAO_INCIAL |ESTADO_ESPERA | ESTADO_VOO
}aviao, *pAviao;

typedef struct {
	aviao planeBuffer[TAM_BUFFER_CIRCULAR];
	int proxEscrita;
	int proxLeitura;
	int maxPos;
} AviaoEControl;

void escreveNoRegistry(HKEY chave, TCHAR par_nome[], TCHAR par_valor[]);
void closeProgram(TCHAR mesage[]);
int obtemIntDoregistry(HKEY chaveRegistry, TCHAR par_nome[]);
void criarAeroporto();
void criaConeccaoComAvioes();
void verificaUnicidade();
void ObtemValoresDoRegistry();

//thread methods
DWORD WINAPI trataBufferAvioes(LPVOID lpParam);


//geral do programa
HKEY chaveRegistry = NULL;
HANDLE verificador;
//fim
boolean turnOffFlag = FALSE;


int max_aeroportos;		//define número máximo de aeroportos
int max_avioes;		//define número máximo de aviões

//cenas do mapa em si
pAeroporto listaAeroportos;	//lista que guarda os aeroportos
int numAeroportos;			//número de aeroportos que existem (último aeroporto criado -> listaAeroportos[numAeroportos - 1])
pAviao listaAvioes;
int numAvioes;
pPassageiro listaPassageiros;
int numPassageiros;

//passageiros
HANDLE atendePassageirosEvent, atendePassageirosMutex;
TCHAR* mapViewOfFileEventoPassageiros;

//Aviões
//atendimento dos aviões
HANDLE hLimiteAvioesConectados;		//limita número de aviões que se podem conectar
//aviao2Control
AviaoEControl* aviao2Control;			//aviao -> control
HANDLE hMutexAviao2Control;				//mutex para escrever no Aviao2Control
HANDLE hSemafotoEscritaAviao2Control;	//semaforo para avioes poderem escrever no Aviao2Control
HANDLE hSemafotoLeituraAviao2Control;	//semaforo para o Control poder ler o Aviao2Control
HANDLE hAviao2Control;					//handle para o buffer circular
//Control2Aviao
AviaoEControl* control2Aviao;			//control -> aviao
HANDLE hMutexControl2Aviao;				//mutex para escrever no Control2Aviao
HANDLE hSemafotoEscritaControl2Aviao;	//semaforo para control poder escrever no Control2Aviao
HANDLE hSemafotoLeituraControl2Aviao;	//semaforo para avioes poderem ler no Control2Aviao
HANDLE hControl2Aviao;					//handle para o buffer circular

//ACEDER A VARIAVEIS LOCAIS
HANDLE hMutexAcederAeroportos;
HANDLE hMutexAcederAvioes;
HANDLE hMutexAcederPassageiros;


int _tmain(int argc, TCHAR* argv[]) {
	
	

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	//verifica se já existe uma instancia a correr ou não
	verificaUnicidade();

	ObtemValoresDoRegistry();
	


_tprintf(TEXT("\nnum max de aviões: %d"), max_avioes);
_tprintf(TEXT("\nnum max de aeroportos: %d"), max_aeroportos);
_tprintf(TEXT("\n"));

listaAeroportos = malloc(max_aeroportos * sizeof(aeroporto));
numAeroportos = 0;
listaAvioes = malloc(max_avioes * sizeof(aeroporto));
numAvioes = 0;
listaPassageiros = malloc(MAX_PASSAGEIROS * sizeof(aeroporto));
numPassageiros = 0;

hMutexAcederPassageiros = CreateMutex(NULL, FALSE, MUTEX_ACEDER_LISTA_PASSAGEIROS);
hMutexAcederAeroportos = CreateMutex(NULL, FALSE, MUTEX_ACEDER_LISTA_AEROPORTOS);
hMutexAcederAvioes = CreateMutex(NULL, FALSE, MUTEX_ACEDER_LISTA_AVIOES);

if (hMutexAcederAeroportos == NULL || hMutexAcederAvioes == NULL) {
	_tprintf(TEXT("Erro a hMutexAcederAeroportos ou hMutexAcederAvioes -> %d"), GetLastError());
	exit(1);
}

//cria threads extra----------------------------------------------------------------

//CRIA THREAD EVENTO E TUDO PARA CONEÇÃO COM OS AVIÕES
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
turnOffFlag = TRUE;

CloseHandle(verificador);	//elimina a handle para que permita que o controlador possa ser ligado multiplas vezes
return 0;
}


//verificação inicial do registry
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
		_tprintf(TEXT("\nNão é possivel criar mais aeroportos!\n"));
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
				_tprintf(TEXT("\nEste nome <%s> já existe, coloque outro"), nome);
				flag = FALSE;
			}
		}
	}

	//obtem coordenadas
	flag = FALSE;
	int x, y;
	while (!flag) {
		flag = TRUE;
		_tprintf(TEXT("\nindique posição X (0 - 999):\t"));
		_tscanf_s(TEXT("%d"), &x, TAM);

		_tprintf(TEXT("\nindique posição Y (0 - 999):\t"));
		_tscanf_s(TEXT("%d"), &y, TAM);

		if (x > 999 || x < 0){
			_tprintf(TEXT("\nValor de X inválido"));
			flag = FALSE;
		}
		else if (y > 999 || y < 0) {
			_tprintf(TEXT("\nValor de Y inválido"));
			flag = FALSE;
		}
		else
			for (int i = 0; i < numAeroportos; i++) {
				long distancia = sqrt(pow((listaAeroportos[i].x - x), 2) + pow((listaAeroportos[i].y - y), 2));	//distancia entre dois pontos

				if (distancia > 10) {
					_tprintf(TEXT("\nCoordenadas do aeroporto a menos de 10 casas do aeroporto mais próximo"));
					flag = FALSE;
				}
			}
	}

	//coloca os valores na posição devida
	wcscpy_s(listaAeroportos[numAeroportos].nome,TAM, nome);
	listaAeroportos[numAeroportos].x = x;
	listaAeroportos[numAeroportos].y = y;
	
	//Aumenta o counter
	WaitForSingleObject(hMutexAcederAeroportos, INFINITE);
	numAeroportos++;									
	ReleaseMutex(hMutexAcederAeroportos);


}

//Generalist Error Methods
void closeProgram(TCHAR mesage[]) {
	if (chaveRegistry != NULL && RegCloseKey(chaveRegistry) == ERROR_SUCCESS)
		_tprintf(TEXT("\nErro crítico! Mensagem de erro:\t%s\nA sair..."), mesage);
	else
		_tprintf(TEXT("\nErro a fechar chave de registo!\n\nA DESLIGAR NA MESMA!"));

	CloseHandle(verificador);	//elimina a handle para que permita que o controlador possa ser ligado multiplas vezes
			
	exit(0);
}

/*
//Thread Methods
DWORD WINAPI atendePassageiros(LPVOID lpParam) {
	//MAPEA PASSAGEIROS NA MEMÓRIA
	HANDLE hFileMapping = CreateFileMappingA(
		INVALID_HANDLE_VALUE,		//usado para quando não há um ficheiro a abrir, é só na ram
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(passageiro),						//coloca lá os dados do passageiro
		WAIT_FOR_PASSAGEIRO_TO_CONNECT_MAP_OF_FILE
	);

	if (hFileMapping == NULL) {
		_tprintf(TEXT("\nErro a mapear ficheiro < %s > em memória-> %d"), WAIT_FOR_PASSAGEIRO_TO_CONNECT_MAP_OF_FILE, GetLastError());
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

	while(!turnOffFlag) {			//isto é uma maneira horrivel para fazer com que o progranma desligue
		WaitForSingleObject(atendePassageirosEvent, INFINITE);
		
		WaitForSingleObject(atendePassageirosMutex, INFINITE);
			
		pPassageiro newPassageiro = mapViewOfFileEventoPassageiros;


		ReleaseMutex(atendePassageirosMutex);
	
		ResetEvent(atendePassageirosEvent);
	}


}
*/


void verificaUnicidade() {

	verificador = CreateFileMappingA(
		INVALID_HANDLE_VALUE,		//usado para quando não há um ficheiro a abrir, é só na ram
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(TCHAR),
		ALREADY_CREATED_FLAG
	);
	if (verificador == NULL) {
		_tprintf(TEXT("Não foi possivel verificar se existia outra instancia a correr\nA desligar"));
		return 1;
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS) {
		_tprintf(TEXT("Já se encontra ligada uma instancia do controlador\nA desligar"));
		return 1;
	}

}

void ObtemValoresDoRegistry() {

	TCHAR chave_nome[TAM], par_nome[TAM], par_valor[TAM];
	TCHAR chave_avioes[] = TEXT("Software\\SO2\\TPMDMF\\AVIOES\\");
	TCHAR chave_aeroportos[] = TEXT("Software\\SO2\\TPMDMF\\AEROPORTOS\\");


	//Obtem valores para aviões
	if (RegCreateKeyEx(HKEY_CURRENT_USER, chave_avioes, 0, NULL, REG_OPTION_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &chaveRegistry, NULL) != ERROR_SUCCESS)
	{
		_tprintf(TEXT("\nErro critico a ler chave de aviões!\nA desligar programa"));
		exit(-1);
	}
	//tenta obter
	wcscpy_s(par_nome, TAM, TEXT("MAXAVIOES"));
	max_avioes = obtemIntDoregistry(chaveRegistry, par_nome);
	if (max_avioes == -1)	//caso não exista para obter, escreve
	{
		wcscpy_s(par_nome, TAM, TEXT("MAXAVIOES"));
		_stprintf_s(par_valor, TAM, TEXT("%d"), MAX_AVIOES);
		escreveNoRegistry(chaveRegistry, par_nome, par_valor);
		max_avioes = MAX_AVIOES;
	}


	//fecha a handle de aviões
	CloseHandle(chaveRegistry);

	if (RegCreateKeyEx(HKEY_CURRENT_USER, chave_aeroportos, 0, NULL, REG_OPTION_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &chaveRegistry, NULL) != ERROR_SUCCESS) {
		_tprintf(TEXT("\nErro critico a ler a chave do controlador2!\nA desligar o programa"));
		exit(-1);
	}
	//tenta obter
	wcscpy_s(par_nome, TAM, TEXT("MAXAEROPORTOS"));
	max_aeroportos = obtemIntDoregistry(chaveRegistry, par_nome);
	if (max_aeroportos == -1)//caso não exista para obter, escreve 
	{
		wcscpy_s(par_nome, TAM, TEXT("MAXAEROPORTOS"));
		_stprintf_s(par_valor, TAM, TEXT("%d"), MAX_AEROPORTOS);
		escreveNoRegistry(chaveRegistry, par_nome, par_valor);
		max_aeroportos = MAX_AEROPORTOS;
	}

	//fecha a handle de aeroportos
	CloseHandle(chaveRegistry);

}

void criaConeccaoComAvioes() {
	//LIMITADOR DE CONEÇÕES 
	hLimiteAvioesConectados = CreateSemaphore(NULL, max_avioes, max_avioes, SEMAFORO_PARA_LIMITE_DE_CONECOES_AVIOES);
	if (hLimiteAvioesConectados == NULL) {
		_tprintf(TEXT("Erro a crear semaforo limitador de coneçoes-> %d\n"), GetLastError());
		return -1;
	}


	/* CONTROL2AVIAO
	AviaoEControl* control2Aviao;			//control -> aviao
	HANDLE hMutexControl2Aviao;				//mutex para escrever no Control2Aviao
	HANDLE hSemafotoEscritaControl2Aviao;	//semaforo para control poder escrever no Control2Aviao
	HANDLE hSemafotoLeituraControl2Aviao;	//semaforo para avioes poderem ler no Control2Aviao
	HANDLE hControl2Aviao;					//handle para o buffer circular
	*/
	hMutexControl2Aviao = CreateMutex(NULL, FALSE, MUTEX_CONTROL2AVIAO);
	hSemafotoEscritaControl2Aviao = CreateSemaphore(NULL, TAM_BUFFER_CIRCULAR, TAM_BUFFER_CIRCULAR, SEMAFORO_ESCRITA_CONTROL2AVIAO);
	hSemafotoLeituraControl2Aviao = CreateSemaphore(NULL, 0, TAM_BUFFER_CIRCULAR, SEMAFORO_LEITURA_CONTROL2AVIAO);

	if (hMutexControl2Aviao == NULL || hSemafotoEscritaControl2Aviao == NULL || hSemafotoLeituraControl2Aviao == NULL) {
		_tprintf(TEXT("Erro no CreateSemaphore ou no CreateMutex -> %d\n"), GetLastError());
		return -1;
	}

	hControl2Aviao = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(AviaoEControl), //tamanho da memoria partilhada
		BUFFER_CONTROL2AVIAO);//nome do filemapping. nome que vai ser usado para partilha entre processos

	if (hControl2Aviao == NULL) {
		_tprintf(TEXT("Erro no CreateFileMapping -> %d\n"), GetLastError());
		return -1;
	}

	control2Aviao = (AviaoEControl*)MapViewOfFile(hControl2Aviao, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	control2Aviao->proxEscrita = 0;
	control2Aviao->proxLeitura = 0;
	control2Aviao->maxPos = TAM_BUFFER_CIRCULAR;



	/*	AVIAO 2 CONTROL
	AviaoEControl* aviao2Control;			//aviao -> control
	HANDLE hMutexAviao2Control;				//mutex para escrever no Aviao2Control
	HANDLE hSemafotoEscritaAviao2Control;	//semaforo para avioes poderem escrever no Aviao2Control
	HANDLE hSemafotoLeituraAviao2Control;	//semaforo para o Control poder ler o Aviao2Control
	HANDLE hAviao2Control;					//handle para o buffer circular
	*/

	hMutexAviao2Control = CreateMutex(NULL, FALSE, MUTEX_AVIAO2CONTROL);
	hSemafotoEscritaAviao2Control = CreateSemaphore(NULL, TAM_BUFFER_CIRCULAR, TAM_BUFFER_CIRCULAR, SEMAFORO_ESCRITA_CONTROL2AVIAO);
	hSemafotoLeituraAviao2Control = CreateSemaphore(NULL, 0, TAM_BUFFER_CIRCULAR, SEMAFORO_LEITURA_CONTROL2AVIAO);

	if (hMutexAviao2Control == NULL || hSemafotoEscritaAviao2Control == NULL || hSemafotoLeituraAviao2Control == NULL) {
		_tprintf(TEXT("Erro no CreateSemaphore ou no CreateMutex -> %d\n"), GetLastError());
		return -1;
	}

	//criamos o bloco de memoria partilhada
	hAviao2Control = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(AviaoEControl), //tamanho da memoria partilhada
		BUFFER_AVIAO2CONTROL);//nome do filemapping. nome que vai ser usado para partilha entre processos

	if (hAviao2Control == NULL) {
		_tprintf(TEXT("Erro no CreateFileMapping\n"));
		return -1;
	}


	aviao2Control = (AviaoEControl*)MapViewOfFile(hAviao2Control, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	aviao2Control->proxEscrita = 0;
	aviao2Control->proxLeitura = 0;
	aviao2Control->maxPos = TAM_BUFFER_CIRCULAR;

	//lança  a thread que comunica com os avioes
	HANDLE  threadConectaAvioes = CreateThread(NULL, 0, trataBufferAvioes, NULL, 0, NULL);
	if (threadConectaAvioes == NULL) {
		_tprintf(TEXT("Erro a criar thread para o buffer circular-> %d\n"), GetLastError());
		exit(-99);
	}

}




/*
	Esta thread usa 2 memorias partilhadas
	Aviao2Control -> onde os aviões escrevem a sua info
	Control2Aviao -> onde o control dá a resposta aos aviões

	Os aviões conectam-se as mesmas, sendo que so escrevem na Aviao2Control
	Quando se conectam colocam o estado ESTADO_CONECAO_INCIAL e o nome do aeroporto inicial no 'destino'
	A thread testa se o estado for ESTADO_CONECAO_INCIAL e o nome do aeroporto inicial existir então adiciona o avião à lista com as coordenadas corretas
	O estado passa para ESTADO_VOO e escreve no buffer Control2Aviao
	Caso o nome do aerporto estiver mal escreve de volta no Control2Aviao Assim, amantendo o estado

	O aviao le o ESTADO_ESPERA e sabe que tem de colocar o nome do aeroporto para onde tem de ir
	De seguida volta a colocar o avião com ESTADO_ESPERA e com o nome do destino na var 'destino' no buffer Aviao2Control

	Ao ler o ESTADO_ESPERA o control verifica se existe um aeroporto com esse nome
		Se não existir: devolve o que recebeu (mantem estado espera)
		Caso Contrario coloca as coordenadas no avião
		Colocaas pessoas no avião
		Escreve o Aviao no Control2Aviao com estado a ESTADO_VOO

	O avião lê o ESTADO_VOO e faz os calculos para a posição seguinte, colocando nas cordenas POS a posição "atual" e escreve no Aviao2Control

	O control, ao ler ESTADO_VOO verifica se alguem já esta nessas coordenadas
		SE JÁ ESTIVER ALGUEM: escreve no Control2Aviao o avião com as coordenadas antigas
		Se não: atualiza as coordenadas no array interno, verifica se chegou ao destino e escreve com essas coordenadas no Control2Aviao
			Se ja tiver chegado ao destino POS == DEST, entao remove os passageiros e troca para o estado ESTADO_ESPERA

*/
DWORD WINAPI trataBufferAvioes(LPVOID lpParam) {
	//espera ter lugar no semaforo para escrita
	//coloca dados iniciais no buffer circular:
	
	int idCounter = 0;
	
	while (!turnOffFlag) {
		aviao data;
		//espera para poder ler os aviões do buffer
		WaitForSingleObject(hSemafotoLeituraAviao2Control ,INFINITE);	//espera semaforo
		WaitForSingleObject(hMutexAviao2Control, INFINITE);				//espera ter acesso à info de maneira segura
		
		memcpy(&data, &aviao2Control->planeBuffer[aviao2Control->proxLeitura], sizeof(data));	//copia a informação para si
		aviao2Control->proxLeitura == aviao2Control->maxPos ? aviao2Control->proxLeitura = 0 : aviao2Control->proxLeitura++;	//aumenta contador da proxima leitura
		
		ReleaseMutex(hMutexAviao2Control);								//liberta o mutex
		ReleaseSemaphore(hSemafotoEscritaAviao2Control, 1, NULL);		//indica que pode escrever mais um!

		//ve estado do aviao e atua conforme
		if (data.estado == ESTADO_CONECAO_INCIAL) {
			_tprintf(TEXT("Um avião conectou-se\n"));
			data.id = idCounter++;			//coloca um id e acrescenta o counter do id

			boolean flagAerportoExiste = FALSE;
			WaitForSingleObject(hMutexAcederAeroportos, INFINITE);		//pede mutex para aceder aos aeroportos
			for (int i = 0; i < numAeroportos; i++) {
				if (wcscmp(listaAeroportos[i].nome, data.destino)==0) {
					flagAerportoExiste == TRUE;
					data.startX = listaAeroportos[i].x;
					data.startY = listaAeroportos[i].y;
					data.posX = listaAeroportos[i].x;
					data.posY = listaAeroportos[i].y;

					break;
				}
			}

			ReleaseMutex(hMutexAcederAeroportos);						//liberta o mutex para aceder aos aeroportos
			//caso o aeroporto exista, pega nas pessoas e leva-as para estado de voo
			if (flagAerportoExiste) {
				/*
				WaitForSingleObject(hMutexAcederPassageiros, INFINITE);
				for (int i = 0; i < numPassageiros; i++)
				{
					if(listaPassageiros[i].)
				}
				ReleaseMutex(hMutexAcederPassageiros);
				*/
				data.estado = ESTADO_VOO;		//mete o aviao a voar
			}
			
			//escreve o aviao no Control2Aviao
			WaitForSingleObject(hSemafotoEscritaControl2Aviao, INFINITE);
			WaitForSingleObject(hMutexControl2Aviao, INFINITE);

			memcpy( &control2Aviao->planeBuffer[aviao2Control->proxEscrita], &data, sizeof(data));	//copia a informação para si
			control2Aviao->proxEscrita == control2Aviao->maxPos ? control2Aviao->proxEscrita = 0 : control2Aviao->proxEscrita++;	//aumenta contador da proxima Escrita

			ReleaseMutex(hMutexControl2Aviao);									//liberta mutex de acesso a aviao
			ReleaseSemaphore(hSemafotoLeituraControl2Aviao, 1, NULL);			//liberta semaforo para leitura

			//coloca o avião na lista
			WaitForSingleObject(hMutexAcederAvioes, INFINITE);
			memcpy(&data, &listaAvioes[numAvioes++], sizeof(data));
			ReleaseMutex(hMutexAcederAvioes);
		}
		else if (data.estado == ESTADO_ESPERA) {
			_tprintf(TEXT("Um avião esta em estado de espera\n"));
			boolean flagAerportoExiste = FALSE;

			WaitForSingleObject(hMutexAcederAeroportos, INFINITE);		//pede mutex para aceder aos aeroportos
			for (int i = 0; i < numAeroportos; i++) {
				if (wcscmp(listaAeroportos[i].nome, data.destino) == 0) {
					flagAerportoExiste == TRUE;
					data.destX = listaAeroportos[i].x;
					data.destY = listaAeroportos[i].y;
					break;
				}
			}
			ReleaseMutex(hMutexAcederAeroportos);						//liberta o mutex para aceder aos aeroportos

			//caso o aeroporto exista, pega nas pessoas e leva-as para estado de voo
			if (flagAerportoExiste) {
				/*
					embarca pessoas
				*/
				data.estado = ESTADO_VOO;		//mete o aviao a voar
			}

			//Atualiza avião na lista
			WaitForSingleObject(hMutexAcederAvioes, INFINITE);

			for (int i = 0; i < numAvioes; i++) {
				listaAvioes[i].destX = data.destX;
				listaAvioes[i].destY = data.destY;
				wcscpy(listaAvioes[i].destino, data.destino);
			}

			ReleaseMutex(hMutexAcederAvioes);


			//escreve o aviao no Control2Aviao
			WaitForSingleObject(hSemafotoEscritaControl2Aviao, INFINITE);
			WaitForSingleObject(hMutexControl2Aviao, INFINITE);

			memcpy(&control2Aviao->planeBuffer[aviao2Control->proxEscrita], &data, sizeof(data));	//copia a informação para si
			control2Aviao->proxEscrita == control2Aviao->maxPos ? control2Aviao->proxEscrita = 0 : control2Aviao->proxEscrita++;	//aumenta contador da proxima Escrita

			ReleaseMutex(hMutexControl2Aviao);									//liberta mutex de acesso a aviao
			ReleaseSemaphore(hSemafotoLeituraControl2Aviao, 1, NULL);			//liberta semaforo para leitura

		}
		else if (data.estado == ESTADO_VOO) {
			_tprintf(TEXT("Um avião esta em voo! { %d ; &d }\n"), data.posX, data.posY);
			boolean flagPosicaoUsada = FALSE;

			WaitForSingleObject(hMutexAcederAvioes, INFINITE);		//ESPERA PARA ACEDER A LISTA DE AVIÕES
			for (int i = 0; i < numAvioes; i++) {
				if (data.posX == listaAvioes[i].posX && data.posY == listaAvioes[i].posY) {
					flagPosicaoUsada = TRUE;					//alguem esta naquela posição
					break;
				}
			}
			ReleaseMutex(hMutexAcederAvioes);

			if (!flagPosicaoUsada) {		//caso a posição esteja livre
				//Atualiza avião na lista
				WaitForSingleObject(hMutexAcederAvioes, INFINITE);
				
				for (int i = 0; i < numAvioes; i++) {
					listaAvioes[i].posX = data.posX;
					listaAvioes[i].posY = data.posY;
				}

				ReleaseMutex(hMutexAcederAvioes);

				//verifica se já chegou ao objetivo
				if (data.posX == data.destX && data.posY == data.destY) {
					/*
						desembarca pessoas
					*/
					data.estado = ESTADO_ESPERA;
				}

			}
			//escreve o aviao no Control2Aviao
			WaitForSingleObject(hSemafotoEscritaControl2Aviao, INFINITE);
			WaitForSingleObject(hMutexControl2Aviao, INFINITE);

			memcpy(&control2Aviao->planeBuffer[aviao2Control->proxEscrita], &data, sizeof(data));	//copia a informação para si
			control2Aviao->proxEscrita == control2Aviao->maxPos ? control2Aviao->proxEscrita = 0 : control2Aviao->proxEscrita++;	//aumenta contador da proxima Escrita

			ReleaseMutex(hMutexControl2Aviao);									//liberta mutex de acesso a aviao
			ReleaseSemaphore(hSemafotoLeituraControl2Aviao, 1, NULL);			//liberta semaforo para leitura


		}


	}
		
		
}

