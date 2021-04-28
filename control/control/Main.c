#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#define TAM 200
#define ALREADY_CREATED_FLAG TEXT("este_valor_e_usado_para_garantir_que_so_existe_uma_instancia_a_correr_de_cada_vez")


typedef struct {
	TCHAR nome[TAM];
	int x, y;
}aeroporto, *pAeroporto;

void escreveNoRegistry(HKEY chave, TCHAR par_nome[], TCHAR par_valor[]);
void closeProgram(TCHAR mesage[]);
int obtemIntDoregistry(HKEY chaveRegistry, TCHAR par_nome[]);
void criarAeroporto();





HKEY chaveRegistry = NULL;



int max_aeroportos = 5;		//define número máximo de aeroportos
int max_avioes = 20;		//define número máximo de aviões

int mapa[1000][1000];		//mapa com cordenadas
pAeroporto listaAeroportos;	//lista que guarda os aeroportos
int numAeroportos;			//número de aeroportos que existem (último aeroporto criado -> listaAeroportos[numAeroportos - 1])


int _tmain(int argc, TCHAR* argv[]) {
	
	TCHAR chave_nome[TAM], par_nome[TAM], par_valor[TAM];
	/* ... Mais variáveis ... */
	TCHAR value_obtained[TAM], value_name_obtained[TAM];
	DWORD size = TAM, data_size = TAM;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	//verifica se já existe uma instancia a correr ou não
	HANDLE verificador = CreateFileMappingA(
		INVALID_HANDLE_VALUE,		//usado para quando não há um ficheiro a abrir, é só na ram
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(TCHAR),
		ALREADY_CREATED_FLAG
	);
	if (verificador == NULL ) {
		_tprintf(TEXT("Não foi possivel verificar se existia outra instancia a correr\nA desligar"));
		return 1;
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS) {
		_tprintf(TEXT("Já se encontra ligada uma instancia do controlador\nA desligar"));
		return 1;
	}


	TCHAR chave_avioes[] = TEXT("Software\\SO2\\TPMDMF\\AVIOES\\");		
	TCHAR chave_aeroportos[] = TEXT("Software\\SO2\\TPMDMF\\AEROPORTOS\\");
	DWORD openChecker;


	//Obtem valores para aviões
	if (RegCreateKeyEx(HKEY_CURRENT_USER, chave_avioes, 0, NULL, REG_OPTION_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &chaveRegistry, &openChecker) != ERROR_SUCCESS)
	{
		_tprintf(TEXT("\nErro critico a ler chave de aviões!\nA desligar programa"));
		exit(-1);
	}
	if (openChecker == REG_CREATED_NEW_KEY) {		//a chave foi criada
		//define max aviões no registry
		wcscpy_s(par_nome, TAM, TEXT("MAXAVIOES"));
		_stprintf_s(par_valor, TAM, TEXT("%d"), max_avioes);
		escreveNoRegistry(chaveRegistry, par_nome, par_valor);
	}
	else{
		wcscpy_s(par_nome, TAM, TEXT("MAXAVIOES"));
		max_avioes = obtemIntDoregistry(chaveRegistry, par_nome);
	}

	//fecha a handle de aviões
	CloseHandle(chaveRegistry);
	openChecker = NULL;		//limpa openchecker

if (RegCreateKeyEx(HKEY_CURRENT_USER, chave_aeroportos, 0, NULL, REG_OPTION_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &chaveRegistry, &openChecker) != ERROR_SUCCESS) {
	_tprintf(TEXT("\nErro critico a ler a chave do controlador2!\nA desligar o programa"));
	exit(-1);
}
if (openChecker == REG_CREATED_NEW_KEY) {		//a chave foi criada
	//define max aeroportos no registry
	wcscpy_s(par_nome, TAM, TEXT("MAXAEROPORTOS"));
	_stprintf_s(par_valor, TAM, TEXT("%d"), max_aeroportos);
	escreveNoRegistry(chaveRegistry, par_nome, par_valor);
}
else {
	wcscpy_s(par_nome, TAM, TEXT("MAXAEROPORTOS"));
	max_aeroportos = obtemIntDoregistry(chaveRegistry, par_nome);		//MAX AEROPORTOS FICA COM O VALOR DO REGISTRY
}
//fecha a handle de aeroportos
CloseHandle(chaveRegistry);


_tprintf(TEXT("\nnum max de aviões: %d"), max_avioes);
_tprintf(TEXT("\nnum max de aeroportos: %d"), max_aeroportos);
_tprintf(TEXT("\n"));

listaAeroportos = malloc(max_aeroportos * sizeof(aeroporto));
numAeroportos = 0;

//cria threads extra----------------------------------------------------------------
//to do


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
	{
		return _tstoi(value_obtained);
	}
	else {
		TCHAR mesage[TAM];
		_stprintf_s(mesage, TAM, TEXT("\nFalha a obter do resgistry chave-valor com nome <%s>"), par_nome);
		closeProgram(mesage);
	}

	return 0;
}

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
					break;
				}
			}
	}

	//coloca os valores na posição devida
	wcscpy_s(listaAeroportos[numAeroportos].nome,TAM, nome);
	listaAeroportos[numAeroportos].x = x;
	listaAeroportos[numAeroportos].y = y;
	numAeroportos++;									//Aumenta o counter


}

void closeProgram(TCHAR mesage[]) {
	if (chaveRegistry != NULL && RegCloseKey(chaveRegistry) == ERROR_SUCCESS)
	{
		_tprintf(TEXT("\nErro crítico! Mensagem de erro:\t%s\nA sair..."), mesage);
		exit(0);
	}
	else
	{
		_tprintf(TEXT("\nErro a fechar chave de registo!\n\nA DESLIGAR NA MESMA!"));
		exit(0);
	}

}
