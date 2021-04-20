#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>

void escreveNoRegistry(HKEY chave, TCHAR par_nome[], TCHAR par_valor[]);
void closeProgram(TCHAR mesage[]);
int obtemIntDoregistry(HKEY chaveRegistry, TCHAR par_nome[]);


#define TAM 200

HKEY chaveRegistry = nullptr;
HKEY chaveRegistry2 = nullptr;


int max_aeroportos = 5;
int max_avioes = 20;

int mapa[1000][1000];


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

	TCHAR chave_controlador[TAM] = TEXT("Software\\SO2\\TPMDMF\\CONTROL\\");		//chave registry para obter valores do controlador
	chave_controlador[TAM - 1] = TEXT('\0');
	TCHAR chave_aeroportos[TAM] = TEXT("Software\\SO2\\TPMDMF\\AEROPORTOS\\");		//chave do gestistry para o obter info de aeroportos (escrve-los lá lmao)
	chave_aeroportos[TAM - 1] = TEXT('\0');


	DWORD openChecker, openChecker2;
	//obtem ou define valores máximos para aeroporto e aviões;
	//mudar o RegCreateKeyEx para RegOpenKeyEx
	if (RegCreateKeyEx(HKEY_CURRENT_USER, chave_controlador, 0, NULL, REG_OPTION_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &chaveRegistry, &openChecker) != ERROR_SUCCESS)
	{
		_tprintf(TEXT("\nErro critico a ler chave de controlador!\nA desligar programa"));
		exit(-1);
	}
	if (RegCreateKeyEx(HKEY_CURRENT_USER, chave_aeroportos, 0, NULL, REG_OPTION_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &chaveRegistry2, &openChecker2) != ERROR_SUCCESS) {
		_tprintf(TEXT("\nErro critico a ler a chave do controlador2!\nA desligar o programa"));
		exit(-1);
	}

	if (openChecker == REG_CREATED_NEW_KEY && openChecker2 == REG_CREATED_NEW_KEY) {		//caso tenham sido criadas -> escreve valores por default
		//define max aeroportos
		wcscpy_s(par_nome,TAM, TEXT("MAXAEROPORTOS"));
		_stprintf_s(par_valor,TEXT("%s"), max_aeroportos);
		escreveNoRegistry(chaveRegistry, par_nome, par_valor);
		//define max aviões
		wcscpy_s(par_nome,TAM, TEXT("MAXAVIOES"));
		_stprintf_s(par_valor, TEXT("%s"), max_avioes);
		escreveNoRegistry(chaveRegistry, par_nome, par_valor);
	}
	else											//caso tenha criado apenas uma das chaves
	{
		if (openChecker == REG_CREATED_NEW_KEY) {	//cria registo dos aviões e lê os aeroportos
			wcscpy_s(par_nome, TAM, TEXT("MAXAEROPORTOS"));
			_stprintf_s(par_valor, TEXT("%s"), max_aeroportos);
			escreveNoRegistry(chaveRegistry, par_nome, par_valor);
			wcscpy_s(par_nome, TAM, TEXT("MAXAVIOES"));
			max_avioes = obtemIntDoregistry(chaveRegistry, par_nome);
		}
		else {										//cria registo dos aeroportos e lê os aviões
			wcscpy_s(par_nome, TAM, TEXT("MAXAVIOES"));
			_stprintf_s(par_valor, TEXT("%s"), max_avioes);
			escreveNoRegistry(chaveRegistry, par_nome, par_valor);
			wcscpy_s(par_nome, TAM, TEXT("MAXAEROPORTOS"));
			max_aeroportos = obtemIntDoregistry(chaveRegistry, par_nome);
		}
		////obtem max aeroportos
		//wcscpy_s(par_nome,TAM, TEXT("MAXAEROPORTOS"));
		//max_aeroportos = obtemIntDoregistry(chaveRegistry, par_nome);
		////obtem max aviões
		//wcscpy_s(par_nome,TAM, TEXT("MAXAVIOES"));
		//max_avioes = obtemIntDoregistry(chaveRegistry, par_nome);			
	}
	
	_tprintf(TEXT("num max de aviões: %d"), max_avioes);
	_tprintf(TEXT("num max de aeroportos: %d"), max_aeroportos);
	


	return 0;
}



void escreveNoRegistry(HKEY chave, TCHAR par_nome[], TCHAR par_valor[]) {
	//adicionar par nome-valor
	if (RegSetValueEx(chave, par_nome, 0, REG_SZ, (LPBYTE)par_valor, sizeof(TCHAR) * (_tcslen(par_valor) + 1)) == ERROR_SUCCESS)
		_tprintf(TEXT("\nPar chave-valor com nome <%s> e valor <%s> escrita com sucesso no registry"), par_nome, par_valor);
	else
	{
		TCHAR mesage[TAM]; 
		_stprintf_s(mesage, TEXT("\nFalha a escrever no resgistry chave-valor com nome <%s> e valor <%s>"), par_nome, par_valor);
		closeProgram(mesage);
	}
}


void closeProgram(TCHAR mesage[]) {
	if (chaveRegistry != nullptr && RegCloseKey(chaveRegistry) == ERROR_SUCCESS)
	{
		_tprintf(TEXT("\nErro crítico!\nMensagem de erro:\n\n%s\nA sair..."), mesage);
		exit(0);
	}
	else
	{
		_tprintf(TEXT("\nErro a fechar chave de registo!\n\nA DESLIGAR NA MESMA!"));
		exit(0);	
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
		_stprintf_s(mesage, TEXT("\nFalha a obter do resgistry chave-valor com nome <%s>"), par_nome);
		closeProgram(mesage);
	}

	return 0;
}
