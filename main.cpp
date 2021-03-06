#include "silaba.h"
#include "fstream"
#include <list>
#include <chrono>
#include <iomanip>
#include <thread>

#ifdef __arm__ // Raspberry Pi
	#include <wiringPi.h>
#elif _WIN32 // Windows
	#include <Windows.h>
#else // Outras plataformas
#endif


#define pb push_back
#define pf push_front
// Adicionados Typedefs por legibilidade.
typedef std::list<Silaba*> Palavra;
typedef std::list<std::list<Silaba*>> Verso;
typedef std::list<std::list<std::list<Silaba*>>> Poema;

int ascii_especial[] = { 192, 193, 194, 195, 199, 201, 202, 205, 211, 212, 213, 218 };
int string_especial[] = { 224, 225, 226, 227, 231, 233, 234, 237, 243, 244, 245, 250 };

vector<Silaba*> Silabas;
vector<Silaba*> Ultimas;
Silaba* espaco = new Silaba(" ");

int maximoSilabas = 10;

const int N_BOTOES = 2;
struct botao {
	int n;
	int estado;
	int atraso;
};
const int PERIODO_MS = 10; // Intervalo em que botoes sao lidos
const int ATRASO_BOUNCE = 80 /	 PERIODO_MS;
const int ATRASO_2BOTOES = 110 / PERIODO_MS;
const int ATRASO_IMPR_MS = 2800;
enum estado_botao { APERTADO, SOLTO, ATIVAR, DESAPERTADO };


/*		INFORMACOES SOBRE OS DICIONARIOS (nltk.corpus usados em geraDicionario.py)
 *	machado: 77000 palavras. 5851 silabas distintas. 3285 terminacoes distintas. Inicializacao estimada em 21000ms. Contem palavras estrangeiras em poucas quantidades.
 *  mac_morpho: 61505 palavras. 8438 silabas distintas. 4849 terminacoes distintas. Inicializacao estimada em 23000ms. Contem palavras estrangeiras em quantidade consideravel.
 *  floresta: 27600 palavras. 5108 silabas distintas. 2272 terminacoes distintas. Inicializacao estimada em 8000ms. Contem palavras estrangeiras em quantidade moderada.
 *  
 *  Contribuicao com novos corpus e com dados sobre tempo de inicializacao sao bem-vindos.
 */

 //Carrega o banco de palavras, gerando o grafo.
void setup(string nomeDoArquivo);
//Gera uma palavra a partir de uma silaba final e com seu maximo tamanho
list<Silaba*> geraPalavra(Silaba* terminacao, int maxTamanho, int linhasAPular);
//Gera o poema, chamando pelas palavras
void geraPoema();
//Nossos MisterEggs. TODO
void sr_ovo1();
void sr_ovo2();
void sr_ovo3();
//Imprime os dados da obra.
void informacoes();

void atualiza_botao(botao* b);
int leitura_botao(int n_botao);
bool dois_botoes(botao b1, botao b2);

void debug_print(string s);

//Converte caracteres especiais (que seriam impressos corrompidos) para sua forma real de caractere
unsigned char converteChar(char caractere);

bool temDicionario = false;



int main(){
    #ifdef __arm__
	wiringPiSetup();
	#endif
	#ifdef __arm__
	setup("/home/beaba/beaba2020/Envio/banco.txt");
	#else
	setup("banco.txt");
	#endif
	srand(time(NULL));
	debug_print("Pronto");
	
	for (int i = 0; i < 12; i++){
		cout << ascii_especial[i] <<endl;
	}


	struct botao botoes[N_BOTOES];
	for (int i = 0; i < N_BOTOES; i++) {
		botoes[i].n = i;
		botoes[i].estado = DESAPERTADO;
		botoes[i].atraso = 0;
	}

	int numInformacoes = 0;
	while (true) {
		#ifdef __arm__
		std::this_thread::sleep_for(std::chrono::milliseconds(PERIODO_MS));
		#endif
		// Estados do botao:
		// DESAPERTADO -> APERTADO -> SOLTO -> ATIVAR -> DESAPERTADO
		// Funcao do botao ativada em ATIVAR
		for (int i = 0; i < N_BOTOES; i++) {
			atualiza_botao(&botoes[i]);
		}

		// Dois botoes soltos ao mesmo tempo
		if (dois_botoes(botoes[0], botoes[1])) {
			cout << "segredo1" << endl;
			//sr_ovo1();
			botoes[0].estado = DESAPERTADO;
			botoes[1].estado = DESAPERTADO;

			#ifdef __arm__
			std::this_thread::sleep_for(std::chrono::milliseconds(ATRASO_IMPR_MS));
			#endif
		}
		else if (botoes[0].estado == ATIVAR) {
			cout << "poema" << endl;
			if (numInformacoes) numInformacoes = 0;
			geraPoema();
			#ifdef __arm__
std::this_thread::sleep_for(std::chrono::milliseconds(ATRASO_IMPR_MS));
#endif
			botoes[1].estado = DESAPERTADO;
		}
		else if (botoes[1].estado == ATIVAR) {
			cout << "informacoes" << endl;
			numInformacoes++;
			if (numInformacoes == 6) sr_ovo2();
			else if (numInformacoes == 13) sr_ovo3();
			else informacoes();
			#ifdef __arm__
std::this_thread::sleep_for(std::chrono::milliseconds(ATRASO_IMPR_MS));
#endif
			botoes[0].estado = DESAPERTADO;
		}
	}
    return 0;
}


void setup(string nomeDoArquivo) 
{

#ifdef DEBUG
	auto begin = chrono::high_resolution_clock::now();
#endif
	//Verifica se eh uma inicializacao ou atualizacao de dicionario.
	if (temDicionario) {
		Silabas.clear();
		Ultimas.clear();
	}
	ifstream Arquivo;
	string ler;
	Arquivo.open(nomeDoArquivo);
    if(Arquivo.fail()) debug_print("Falha na abertura do arquivo.");
	while (Arquivo) {
		Arquivo >> ler;
		string silaba;
		Silaba* silabaO;
		Silaba* anteriorO;
		bool comecoDePalavra = true;
		bool jaExiste = false;
		bool ignoraBit = false;
		for (int i = 0; i < (int)ler.size(); i++) {
			ignoraBit = false;
			ler[i] = tolower(ler[i]); //Trabalhamos apenas com letras minusculas
			int bitsIgnorados[] = {-17, -69, -65};
			for (int j = 0;!ignoraBit && j < 3; j++){
				if ((int)ler[i] == bitsIgnorados[j]) ignoraBit = true;
			}
			if (ignoraBit) continue;
			//Formando a silaba
			if (ler[i] != '-' && ler[i] != '.') {
				silaba += ler[i];
			}
			//Silaba Formada
			else if (silaba.length() < 5) {
				jaExiste = false;
				//Verificando se a silaba ja existe
				unsigned int j;
				for (j = 0; j < Silabas.size() && !(jaExiste); j++) {
					if (silaba == Silabas.at(j)->getEscrita()) jaExiste = true; //Comparacao por nomes
				}

				if (jaExiste) {
					silabaO = Silabas.at(j - 1);
					/*Voltamos para o indice da silaba repetida. Nao podemos fazer j-- diretamente no laco ja que estamos lidando com uma variavel "unsigned".
					  Em caso de j == 0, causariamos underflow de j, o que daria problemas.
					*/
				}
				else {
					silabaO = new Silaba(silaba);
					Silabas.push_back(silabaO);
					debug_print(silabaO->getEscrita());
				}
				silaba.clear();
				if (!comecoDePalavra) {
					silabaO->addPredecessora(anteriorO); //A funcao "addPredecessora()" ja lida com casos de silabas que ja sao predecessoras, adicionando 1 na sua frequencia como predecessora.
				}
				anteriorO = silabaO;

				comecoDePalavra = false; //Passamos de pelo menos uma silaba, logo temos uma silaba anterior.
			}

		}
		if (!jaExiste) Ultimas.pb(silabaO);//silaba final de palavra, usaremos para rimar
		anteriorO = NULL;
		comecoDePalavra = true;
		//Finalizamos uma palavra, logo nao temos silabas antecessoras.
		ler.clear();
	}
	Arquivo.close();
	temDicionario = true;
#ifdef DEBUG
	auto end = chrono::high_resolution_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count();
	cout << "Quantidade de Silabas: "<< (unsigned int) Silabas.size() << "\n" << "Quantidade de Terminacoes: " << (unsigned int) Ultimas.size() << "\n" << "Tempo de Inicializacao: " << (int) ms << "ms\n";
#endif
}

//list<Silaba*> geraPalavra(Silaba* terminacao, int maxTamanho) {
Palavra geraPalavra (Silaba* terminacao, int maxTamanho){
//A opcao por uma list em vez de um vector se deve ao metodo "push_front" que o vector nao tem.
	//list<Silaba*> palavra;
    Palavra palavra;
	//Geramos as palavras da ultima silaba para a primeira
	Silaba* adicionada = terminacao;
	for (int numSilabas = 0; numSilabas < maxTamanho; numSilabas++) {
		palavra.pf(adicionada);

		adicionada = palavra.front()->sorteiaPredecessora();
		//A quantidade de silabas deve ser mantida, mesmo se a silaba encontrada nao tem predecessoras.
		if (adicionada == NULL) {
			adicionada = Silabas.at(rand() % Silabas.size());
		    //break;
        }
	}
	return palavra;
}

void geraPoema()
{
	Silaba* rimas[] = { espaco, espaco, espaco, espaco };

	// Geramos as terminacoes das rimas e tomamos cuidado para que nao sejam repetidas (ja nao ha repeticoes no nosso espaco de silabas)
	rimas[0] = Ultimas.at(rand() % Ultimas.size());
	do {
		rimas[1] = Ultimas.at(rand() % Ultimas.size());
	} while ((rimas[1]->getEscrita() == rimas[0]->getEscrita()));
	do {
		rimas[2] = Ultimas.at(rand() % Ultimas.size());
	} while (rimas[2]->getEscrita() == rimas[0]->getEscrita() || rimas[2]->getEscrita() == rimas[1]->getEscrita());
	do{	rimas[3] = Ultimas.at(rand() % Ultimas.size());
	} while (rimas[3]->getEscrita() == rimas[0]->getEscrita() || rimas[2]->getEscrita() == rimas[1]->getEscrita() || rimas[3]->getEscrita() == rimas[2]->getEscrita());


	//Geracao dos Versos
	//list<list<list<Silaba*>>> poema;
	Poema poema;
    for (int numVerso = 1; numVerso <= 14; numVerso++) {
		int numSilabas = 0;
		int qtdSilabas = 0;
        int qtdSilabasR = 0;
		Verso verso;
        Palavra palavra;
        //list<list<Silaba*>> verso;
		//list<Silaba*> palavra;
		bool temMonossilaba = false;
		while (numSilabas < maximoSilabas) {
			//Geracao da quantidade de silabas. Evita palavras muito grandes.
			do { qtdSilabasR = rand() % (maximoSilabas - numSilabas); 
			} while (qtdSilabasR > 13);

			qtdSilabasR %= maximoSilabas - numSilabas;
            if (qtdSilabasR == 1) qtdSilabas = 1;
            else if (qtdSilabasR > 1 && qtdSilabasR < 5) qtdSilabas = 2;
            else if (qtdSilabasR > 4 && qtdSilabasR < 10) qtdSilabas = 3;
            else if (qtdSilabasR > 9 && qtdSilabasR < 13) qtdSilabas = 4;
            else if (qtdSilabasR == 13) qtdSilabas = 5;
			//Evita muitas monossilabas, reduzindo repeticao de palavras finais.
			if (((temMonossilaba)) && ((qtdSilabas == 1))) { 
				qtdSilabas++; 
			}

			qtdSilabas++;

			if (qtdSilabas == 1 && numSilabas < 8) { 
				temMonossilaba = true; 
			}

			if ((qtdSilabas + numSilabas) >= maximoSilabas) {
				qtdSilabas = maximoSilabas - numSilabas;
				//Esta no fim do verso, deve rimar.
				switch (numVerso) {
				case 1:palavra = (geraPalavra(rimas[0], qtdSilabas)); break; //A
				case 2:palavra = (geraPalavra(rimas[1], qtdSilabas)); break; //B
				case 3:palavra = (geraPalavra(rimas[1], qtdSilabas)); break; //B
				case 4:palavra = (geraPalavra(rimas[0], qtdSilabas)); break; //A
				case 5:palavra = (geraPalavra(rimas[0], qtdSilabas)); break; //A
				case 6:palavra = (geraPalavra(rimas[1], qtdSilabas)); break; //B
				case 7:palavra = (geraPalavra(rimas[1], qtdSilabas)); break; //B
				case 8:palavra = (geraPalavra(rimas[0], qtdSilabas)); break; //A
				case 9:palavra = (geraPalavra(rimas[2], qtdSilabas)); break; //C
				case 10:palavra = (geraPalavra(rimas[3], qtdSilabas)); break; //D 
				case 11:palavra = (geraPalavra(rimas[2], qtdSilabas)); break; //C
				case 12:palavra = (geraPalavra(rimas[3], qtdSilabas)); break; //D
				case 13:palavra = (geraPalavra(rimas[2], qtdSilabas)); break; //C
				case 14:palavra = (geraPalavra(rimas[3], qtdSilabas)); break; //D
				}

			}
			else {
				palavra = (geraPalavra(Ultimas.at(rand() % Ultimas.size()), qtdSilabas));
			}
			verso.pb(palavra);
			numSilabas += palavra.size();
		}
		poema.pb(verso);
		verso.clear();
	}

	ofstream saida;
	saida.open("poema.txt");

	//list<list<list<Silaba*>>>::iterator iteradorExterno = poema.begin();
	Poema::iterator iteradorExterno = poema.begin();
    int numVerso = 1;
	//Iterando pelos versos do poema
	while (iteradorExterno != poema.end()) {
		Verso versoAtual = (*iteradorExterno);
        Verso::iterator iteradorInterno = versoAtual.begin();
        //list<list<Silaba*>> versoAtual = (*iteradorExterno);
		//list<list<Silaba*>>::iterator iteradorInterno = versoAtual.begin();
		//Iterando pelas palavras de cada verso
		while (iteradorInterno != versoAtual.end()) {
			Palavra palavraAtual = (*iteradorInterno);
            Palavra::iterator iteradorPalavra = palavraAtual.begin();
            //list<Silaba*> palavraAtual = (*iteradorInterno);
			//list<Silaba*>::iterator iteradorPalavra = palavraAtual.begin();
			//Iterando pelas silabas de cada palavra
			while (iteradorPalavra != palavraAtual.end()) {
				string aux = (*iteradorPalavra)->getEscrita();
				//Iterando por cada letra de cada silaba, substituindo caracteres especiais para escrever corretamente.
				for (int i = 0; i < aux.length(); i++) {
					#ifdef _WIN32
					/*for (int w = 0; w < 12; w++) {
						if ((int)(unsigned char)aux[i] == string_especial[w]) {
							aux[i] =(unsigned char)ascii_especial[w];
						}
					}*/
					#endif
					aux[i] = converteChar(aux[i]);
					//Inicio do verso com letra maiuscula.
					if (iteradorPalavra == palavraAtual.begin() && i == 0 && iteradorInterno == versoAtual.begin()) saida << (aux[i]);
					else saida << aux[i];
				}

				iteradorPalavra++;
			}
			saida<< " ";
			iteradorInterno++;
			
		}
		saida << "\n";
		if (numVerso == 4 || numVerso == 8 || numVerso == 11 || numVerso == 14) saida << "\n";
		iteradorExterno++;
		numVerso++;
	}
	saida.close();
	#ifdef __arm__
	system("./script-impressao-poema.sh &");
	#else
	system("poema.txt");
	#endif

}

void sr_ovo1()
{
	// Imprime segredo 1
	system("./script-impressao.sh sr-ovo1 &");
}

void sr_ovo2()
{
	// Imprime segredo 2'
	system("./script-impressao.sh sr-ovo2 &");
}

void sr_ovo3()
{
	// Imprime segredo 3
	system("./script-impressao.sh sr-ovo3 &");
}

void informacoes()
{
	//Cabe espa�o pra rodar o programa de latexifica��o das informa��es ou a gente faz direto em latex
	system("./script-impressao.sh informacoes &");
}

void atualiza_botao(botao* b)
{
	switch (b->estado) {
	case APERTADO:
		b->atraso++;
		if (b->atraso > ATRASO_BOUNCE && leitura_botao(b->n) == 0) {
			b->estado = SOLTO;
			b->atraso = 0;
		}
		break;
	case SOLTO:
		b->atraso++;
		if (b->atraso > ATRASO_2BOTOES) {
			b->estado = ATIVAR;
			b->atraso = 0;
		}
		break;
	case ATIVAR:
		b->estado = DESAPERTADO;
		b->atraso = 0;
		break;
	case DESAPERTADO:
		b->atraso++;
		if (b->atraso > ATRASO_BOUNCE && leitura_botao(b->n) == 1) {
			b->estado = APERTADO;
			b->atraso = 0;
		}
		break;
	default:
		break;
	}
}

int leitura_botao(int n_botao)
{
#ifdef _WIN32
	//cout << "Leitura..." << endl;
	switch (n_botao) {
	case 0:
		if (GetKeyState('H') & 0x8000/*Check if high-order bit is set (1 << 15)*/) {
			return 1;
		}
		return 0;
		break;
	case 1:
		if (GetKeyState('J') & 0x8000/*Check if high-order bit is set (1 << 15)*/) {
			return 1;
		}
		return 0;
		break;
	}
#elif defined(__arm__)
	int pino;
	switch (n_botao) {
	case 0:
		pino = 0;
		break;
	case 1:
		pino = 1;
		break;
	}
	return digitalRead(pino);
#endif // __arm__
	return 0;
}

bool dois_botoes(botao b1, botao b2)
{
	return ((b1.estado == ATIVAR && b2.estado == SOLTO) ||
		(b2.estado == ATIVAR && b1.estado == SOLTO) ||
		(b1.estado == ATIVAR && b2.estado == ATIVAR));
}

void debug_print(string s)
{
#ifdef DEBUG
    auto time = std::time(nullptr);	
    cout << std::put_time( std::localtime(&time), "%F %T" ) << " " << s << endl;
#endif // DEBUG
	return;
}

unsigned char converteChar(char caractere){
	for (int w = 0; w < 12; w++) {
		if ((int)(unsigned char) caractere == string_especial[w]) {
			caractere =(unsigned char)ascii_especial[w];
		}
	}
	return (unsigned char)caractere;
}