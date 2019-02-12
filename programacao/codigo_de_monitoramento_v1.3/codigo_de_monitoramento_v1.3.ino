
/* 
 * Instituto Federal de Educação, Ciência e Tecnologia Minas Gerais
 * IFMG - Campus Avançado Conselheiro Lafaiete 
 * 
 * Código para monitoramento de pluviógrafo Versão 1.3
 * 
 * Autor.............: Jonas Henrique Nascimento
 * Data de início....: 30/06/2018
 * Data da ultima atualização: 30/10/2018
 * Data de término...: 30/10/2018
 * 
 * O código consiste em um leitor de dois botões que recebem nível lógico zero ao serem pressionados pelo movimento da báscula.
 * Após a leitura dos botões é feito uma contagem precisa de tempo de 1 minuto, a partir do modulo RTC. Com isso, se obtém o valor da intensidade do pressionar dos botões 
 * sobre o tempo decorrido.
 * Como se espera analisar os valores ao futuro, estes são armazenados em um cartão SD em um arquivo SVC.
 * A intencidade será calculada a partir do número de pulsos colhidos pelos botões por minuto, e esta será calculada e armazenada no cartão SD por forma de um datalogger.
 * Tal datalogger será formado por um modelo de agenda em forma de tabela, contendo a data referênte a medida de cada pulso, e uma contagem do numero total naquele minuto.
 * Paralelamente será medida a temperatura ambiente através de um leitor de temperatura já embutido no shield RTC, a qual será armazenada, também no datalogger.
 * Ademais, o sistema contará com a possibilidade de revisionamentos, podendo alterar a data referente ao RTC por meio de uma interface HM ( Homem-máquina ).
 * Tal interface poderá ser acessada através da comunicação serial entre o microcontrolador e o computador, por meio de um cabo USB e de um adaptador Serial-SPI. Ou por meio da
 * atualização do código no microprocessador 328p.
 * 
 * Todo o código, esquemático do circuito eletrônico e demais informações estarão sempre contidas no endereço abaixo, para livre aperfeiçoamento do código e do circuito. Todavia
 * pede-se, por educação, que ao compartilharem o código, mantenham os autores originais, tão bem quanto o nome da instituição.
 * 
 * https://github.com/W8jonas/pluviografo
 * 
 */


//-----------------------------------------------------------------
// §§§§ Hardware §§§§  //
/* 
 * Consiste na comunicação SPI com o módulo cartão SD, na comunicação I2C com o módulo DS3231. Além de possuir sua saída, originalmente o pino 5 ligado a um 
 * resistor que é ligado à um LED em current source. Ademais, os dois botões estão ligados em pull up. Originalmente, nos digitais 5 e 6.
 * Para mais informações consute os esquemáticos na raiz de todo o projeto, apresentados em versao Proteus e em PDF, seguindo o link do GitHub, apresentado 
 * no cabeçalho deste projeto.
 * 
 */


//----------------------------------------------------------------------
// §§§§ Declaração das bibliotecas §§§§  //
#include <avr/sleep.h>		                                            // Biblioteca para controle de energia
#include <DS3231.h>		                                                // Biblioteca para o Relógio RTC DS3231
#include <SPI.h>		                                                // Biblioteca SPI para comunicação
#include <SD.h>		                                                    // Biblioteca para o módulo cartão SD


//----------------------------------------------------------------------
// §§§§ Definições de Hardware §§§§  //
#define entrada1 6		                                                // Pino digital 6 entrada 
#define entrada2 5		                                                // Pino digital 5 entrada 
#define saida    7		                                                // Pino digital 7 saida, será o pino responsável por ligar o LED indicador
#define interrupcao 2		                                            // Pino digital 2 entrada que será utilizado como interrupção externa.
#define chip_select 4		                                            // Pino digital 4 saida que será utilizado como chip select do modulo SD
#define shield_RTC 8		                                            // Pino digital 8 saida, responsável por chavear o desligamento do shield RTC
#define shield_SD 9		                                                // Pino digital 9 saida, responsável por chavear o desligamento do shield SD


//----------------------------------------------------------------------
// §§§§ Declaração das variáveis globais §§§§  //						// Durante o desenvolvimento do projeto foi adotado a utilização de praticamente todas as váriáveis 
                                                                        // como globais, todavia, caso seja nescessário acrescentar funcionalidades ao código, algumas váriáveis
																		// deverão ter o escopo alteradas.
bool estado1 = false;													// Variável booleana que armazenará o valor da entrada 1
bool estado2 = false;													// Variável booleana que armazenará o valor da entrada 2
bool flag1   = false;													// Variável booleana que armazena a subida de estado da variável 1
bool flag2   = false;													// Variável booleana que armazena a subida de estado da variável 2

volatile byte flag_acordando = false;									// Flag que é setada a True quando o circuito esta acordando.

unsigned int minuto_antigo = 0;											// Responsável por armazenar o minuto anterior ao valor atual
unsigned int minuto_atual = 0;											// Armazena o minuto atual registrado no RTC
unsigned int contador_de_interrupcao = 0;								// Variável que armazena a contagem do número de interrupção antes de 'dormir'
unsigned int contador_de_pulsos_botao_1 = 0;							// Variável auto descritiva
unsigned int contador_de_pulsos_botao_2 = 0;							// Variável auto descritiva
unsigned int contador_de_pulsos = 0;									// Conta a quantidade de pulsos em ambos os botões
unsigned long tempo_atual = 0;											// Variável auxiliar para contagem e controle de tempo
unsigned long tempo_de_delay = 0;										// Variável auxiliar para contagem e controle de tempo
byte ultimo_botao = 0;													// Variável que garante que o mesmo botão não seja apertado duas vezes seguidas
int contador_de_debug = 0;												// Finalidade única para debug do software


//----------------------------------------------------------------------
// §§§§ Declaração dos objetos §§§§  //
DS3231  rtc(SDA, SCL);												    // Objeto RTC para comunicação de valores correspondentes ao módulo DS3231
Time  t;																// Objeto T para obtenção de valores de tempo do módulo DS3231
File datalogger;														// Objeto do tipo FILE para escrever os valores no Cartão SD


//----------------------------------------------------------------------
// §§§§ Declaração de funcoes adicionais §§§§  //
void comecando_a_dormir();												// Responsável por iniciar o modo de economia de energia
void acordando();														// Prepara o arduino para ser inicializado


//----------------------------------------------------------------------
// §§§§ Void Setup §§§§  //
void setup() {
   pinMode(entrada1, INPUT);											// Configura entrada 1   como INPUT
   pinMode(entrada2, INPUT);											// Configura entrada 2   como INPUT
   pinMode(interrupcao, INPUT);											// Configura interrupcao como INPUT
   pinMode(saida, OUTPUT);												// Configura saida 		 como OUTPUT
   pinMode(shield_RTC, OUTPUT);											// Configura shield_RTC  como OUTPUT
   pinMode(shield_SD, OUTPUT);											// Configura shield_SD   como OUTPUT
   pinMode(3, OUTPUT);													// Configura digital 3   como OUTPUT
   Serial.begin(115200);												// Inicializa comunicação serial em 115200 bits por segundo
   int i = 150;															// Inicializa i com 150
   rtc.begin();															// Chamada da função para ligar o módulo RTC
   while (!Serial) {;}													// Aguarda a comunicação serial inicializar
   
   digitalWrite(shield_SD, HIGH);										// Configura shield_RTC como HIGH
   delay(1);															// Aguarda 1 ms
   if (!SD.begin(chip_select)) {										// Testa se o shield SD responde ao chip_select
     Serial.println("Erro ao ler cartao de memoria");					// Caso não responda printa em serial "Erro ao ler cartao de memoria"
     return;															// finaliza o programa
  }
   //rtc.setDOW(FRIDAY);          										// Selecione o dia em ingles; Ex: (SUNDAY) - Domingo
   //rtc.setTime(14, 30, 10);     										// Selecione a hora; Ex: (14, 30, 10)  -- 14 horas, 30 minutos e 10 segundos
   //rtc.setDate(20, 7, 2018);    										// Selecione a data; Ex: (20, 7, 2018) -- Dia 20 do mes 7 de 2018.
   
   while(1){															// Loop para sinalização com o led indicador
      digitalWrite(saida, HIGH);										// configura saida como HIGH
      delay(i);															// espera pino saida em HIGH por um tempo 
      i = i-10;															// Decrementa i em 10 unidades
      digitalWrite(saida, LOW);											// configura saida como LOW
      delay(i);															// espera pino saida em LOW por um tempo 
      if (i < 50) {i = i+9;}											// Caso o i seja menor que 50, será somado 10 ao i, para que ele decremente mais devagar no loop
      if (i == 9) {break;}												// se i = 9, saia do loop
   }
   
   digitalWrite(saida, HIGH);											// configura saida como HIGH para ultima piscada do led indicador
   delay(500);															// espera 500 ms
   digitalWrite(saida, LOW);											// configura pino saida em lowe
   attachInterrupt(0, acordando, RISING);								// Configura interrupção externa em borda de subida para a função acordando
   //detachInterrupt(0);
}


//----------------------------------------------------------------------
// §§§§ Void Loop §§§§  //
void loop() {															// Loop infinito
   digitalWrite(shield_SD, HIGH);										// Chaveia shield_SD para ligado
   digitalWrite(shield_RTC, HIGH);										// Chaveia shield_RTC para ligado

   t = rtc.getTime();  													// objeto t recebe valores de tempo do modulo RTC
   minuto_atual = t.min;												// Minuto atual receve o valor minuto do objeto t 
   
   if (flag_acordando){													// Caso o arduino esteja acordando:
      tempo_de_delay = millis();										// tempo_de_delay recebe o valor da função millis, que retorna o tempo em milissegundos 
      tempo_atual = tempo_de_delay;										// Tempo_atual recebe o valor de tempo_de_delay, ou seja, fica igual ao valor da função millis()
      flag_acordando = false;											// Seta flag_acordando em false;
   }
   
   if(tempo_de_delay != 0){												// Caso o tempo_de_delay seja diferente de zero:
      tempo_de_delay = millis();										// tempo_de_delay se atualiza com o valor da função millis
   }
   
   if (contador_de_debug == 20000){										// Condição para realizar a plotagem de variáveis para debug
      Serial.print("Tempo_atual: ");									// Printa no serial "Tempo_atual: "
      Serial.print(tempo_atual);										// Printa o valor da variável Tempo_atual
      Serial.print("      ");											// Printa um espaço para a próxima palavra
      Serial.print("Tempo_de_delay: ");									// Printa no serial "Tempo_atual: "
      Serial.print(tempo_de_delay);										// Printa o valor da variável tempo_de_delay
      Serial.print("      ");											// Printa um espaço para a próxima palavra
      Serial.print("Diferença de tempo: ");								// Printa no serial "Diferença de tempo: "
      Serial.print(tempo_de_delay - tempo_atual);						// Printa o valor da conta entre as duas variáveis
      Serial.print("      ");											// Printa um espaço para a próxima palavra
      Serial.print(minuto_antigo);										// Printa o valor da variável minuto_antigo
      Serial.print(" ");												// Printa um espaço para a próxima palavra
      Serial.print(minuto_atual);										// Printa o valor da variável minuto_atual
      Serial.println("      ");											// Printa um espaço para a próxima palavra
      contador_de_debug = 0;											// Zera o valor do contador de debug
   }
   //contador_de_debug++;												// Incrimenta o contador_de_debug para a próxima 
   
   if((tempo_de_delay - tempo_atual) < 50000){							// Testa se tempo_de_delay - tempo_atual é menor que 50000. Caso positivo:
      minuto_antigo = minuto_atual;										// Minuto_antigo recebe minuto_atual
   }
   
   if ( minuto_antigo != minuto_atual ){								// Condição caso passe 1 minuto: (Todo o código a seguir será feito quando trocar de minuto)
      flag_acordando = false;											// Seta flag_acordando para false;
      tempo_atual = tempo_de_delay;										// Iguala tempo_atual com tempo_de_delay
      minuto_antigo = minuto_atual;										// Atualiza o minuto anterior para o minuto atual
      
      Serial.println(" ");												// Printa um espaço e pula de linha
      Serial.println(" ");												// Printa um espaço e pula de linha
      Serial.println(" Atualizando datalogger ");						// Printa no serial " Atualizando datalogger "
      Serial.print("O total de pulsos nesse minuto foram de: ");		// Printa no serial "O total de pulsos nesse minuto foram de: "
      Serial.println(contador_de_pulsos);								// Printa no serial a variável contador_de_pulsos
      Serial.print("O total de pulsos do botao 1 foi de: ");			// Printa no serial "O total de pulsos do botao 1 foi de: "
      Serial.println(contador_de_pulsos_botao_1);						// Printa no serial a variável contador_de_pulsos_botao_1
      Serial.print("O total de pulsos do botao 2 foi de: ");			// Printa no serial "O total de pulsos do botao 2 foi de: "
      Serial.println(contador_de_pulsos_botao_2);						// Printa no serial a variável contador_de_pulsos_botao_2
      
      
      if (contador_de_pulsos == 0) {									// Caso passe 1 minuto e nenhum botão tenha sido apertado:
         contador_de_interrupcao++;										// Contador_de_interrupcao incrementa em 1 unidade
      } else {															// Senao:
         contador_de_interrupcao = 0;									// Contador_de_interrupcao zera
      }
      
      datalogger = SD.open("ValorT.svc", FILE_WRITE);					// Ponteiro de escrita datalogger recebe "ValorT.svc" para escrever
      if ( datalogger ) {												// Testa se datalogger esta ativo
         Serial.println("___________________________.DATALOGGER ATT.______________________________");  // Printa mensagem na tela
         Serial.print("DATA: ");										// Printa no serial "DATA: "
         Serial.print(rtc.getDateStr());								// Printa no serial a data atual
         Serial.print("  HORA: ");										// Printa no serial "  HORA: "
         Serial.print(rtc.getTimeStr());								// Printa no serial a hora atual
         Serial.print("   ");											// Printa no serial um espaço em branco
         Serial.println();												// Pula de linha no serial
//         datalogger.println("    Data,    |    Hora,    |   Contagem,   |   Temperatura,   |"); // Linha para tabulação e título em tabela do Excel, executar somente uma vez
         datalogger.print(rtc.getDateStr());							// Escreve no arquivo 'ValorT.svc' a data atual
         datalogger.print(",");											// Escreve no arquivo ","
         datalogger.print(rtc.getTimeStr());							// Escreve no arquivo a data atual
         datalogger.print(",");											// Escreve no arquivo ","
         if(contador_de_pulsos < 10) {datalogger.print("0");}			// Se contador_de_pulsos menor que 10, escreve no arquivo 0 antes do próximo digito
         datalogger.print(contador_de_pulsos);							// Escreve no arquivo o valor da variavel contador_de_pulsos
         if(contador_de_pulsos_botao_1 < 10) {datalogger.print("0");}	// SE contador_de_pulsos_botao_1 menor que 10, escreve no arquivo 0 antes do próximo digito
         datalogger.print(contador_de_pulsos_botao_1);					// Escreve no arquivo o valor da variavel contador_de_pulsos_botao_1
         if(contador_de_pulsos_botao_2 < 10) {datalogger.print("0");}	// SE contador_de_pulsos_botao_2 menor que 10, escreve no arquivo 0 antes do próximo digito
         datalogger.print(contador_de_pulsos_botao_2);					// Escreve no arquivo o valor da variavel contador_de_pulsos_botao_2
         datalogger.print(",");											// Escreve no arquivo ","
         datalogger.print(rtc.getTemp());								// Escreve no arquivo a temperatura atual
         datalogger.println(",");										// Escreve no arquivo ","
         datalogger.close();											// Fecha o ponteiro de escrita do arquivo
      } else {															// Se não estiver ativo
         Serial.println("Erro ao abrir datalogger");					// Printa no serial "Erro ao abrir datalogger"
      }																	
      contador_de_pulsos = 0;											// Zera a variável contador_de_pulsos
      contador_de_pulsos_botao_1 = 0;									// Zera a variável contador_de_pulsos_botao_1
      contador_de_pulsos_botao_2 = 0;									// Zera a variável contador_de_pulsos_botao_2
      Serial.println("Atualizado");										// Printa no serial "Atualizado"
   
   }
   
   estado1 = digitalRead(entrada1);										// Armazena em Estado1 o estado lógico da entrada 1
   estado2 = digitalRead(entrada2);										// Armazena em Estado2 o estado lógico da entrada 2

   if ( estado1 == LOW ) {												// Se estado1 for igual a LOW:
      flag1 = true;														// Então frag1 passa a ser True
   }

   if ( estado2 == LOW ) {												// Se estado2 for igual a LOW:
      flag2 = true;														// Então frag2 passa a ser True
   }

   if ( (estado1 == HIGH) && (flag1 == true) && (ultimo_botao != 1) ){	// Caso a condição seja satisfeita, botão 1 trocou de estado. (botao apertado)
     // while(digitalRead(entrada1) == HIGH){;}							// Linha para debug, desconsiderar no código original
      Serial.println("botao 1 apertado");								// Printa no serial "botao 1 apertado"
      digitalWrite(saida, LOW);											// Deixa saida em Low
      contador_de_pulsos_botao_1++;										// incrementa em 1 a variavel contador_de_pulsos_botao_1
      contador_de_pulsos ++;											// incrementa em 1 a variavel contador_de_pulsos
      digitalWrite(saida, HIGH);										// Deixa saida em HIGH
      delay(20);														// Delay de 20 ms
      digitalWrite(saida, LOW);											// Deixa saida em Low
      delay(20);														// Delay de 20 ms
      flag1 = false;													// Torna flag1 igual a false
      ultimo_botao = 1;													// declara ultimo_botao igual a 1
   }

   if ( (estado2 == HIGH) && (flag2 == true) && (ultimo_botao != 2) ){	// Caso a condição seja satisfeita, botão 1 trocou de estado. (botao apertado)
      Serial.println("botao 2 apertado");								// Printa no serial "botao 2 apertado"
      digitalWrite(saida, HIGH);										// Deixa saida em HIGH
      contador_de_pulsos_botao_2 ++;									// incrementa em 1 a variavel contador_de_pulsos_botao_2
      contador_de_pulsos ++;											// incrementa em 1 a variavel contador_de_pulsos
      digitalWrite(saida, HIGH);										// Deixa saida em HIGH
      delay(20);														// Delay de 20 ms
      digitalWrite(saida, LOW);											// Deixa saida em Low
      delay(20);														// Delay de 20 ms
      flag2 = false;													// Torna flag2 igual a false
      ultimo_botao = 2;													// declara ultimo_botao igual a 2
   }
   
   if ((estado1 == HIGH) && (estado2 == HIGH) && (flag1 == true) && (flag2 == true)) { // Erro, pois não se pode ter os dois botões ou as duas flags setadas ao mesmo tempo.
      Serial.println("Erro lógico, Duas entradas em HIGH");				// Printa no serial "Erro lógico, Duas entradas em HIGH"  {Futura implementacao de watchdog timer}
      datalogger = SD.open ("Valores.svc, FILE_WRITE");					// Ponteiro de escrita datalogger recebe "ValorT.svc" para escrever
      if (datalogger) {													// Testa se datalogger esta ativo
         datalogger.print("______________________________________________________________|");
         Serial.print("DATA: ");										// Printa no serial "DATA: "
         Serial.print(rtc.getDateStr());								// Printa no serial a data atual
         Serial.print("  HORA: ");										// Printa no serial "  HORA: "
         Serial.print(rtc.getTimeStr());								// Printa no serial a hora atual
         Serial.print("   ");											// Printa no serial um espaço em branco
         Serial.println();												// Pula de linha no serial
         if(contador_de_pulsos < 10) {datalogger.print("0");}			// Se contador_de_pulsos menor que 10, escreve no arquivo 0 antes do próximo digito
         datalogger.print(contador_de_pulsos);							// Escreve no arquivo o valor da variavel contador_de_pulsos
         datalogger.print(",");											// Escreve no arquivo ","
         datalogger.print(rtc.getTemp());								// Escreve no arquivo a temperatura atual
         datalogger.println(",");										// Escreve no arquivo ","
         datalogger.print("Erro lógico, Duas entradas em HIGH");		// Escreve no arquivo "Erro lógico, Duas entradas em HIGH"  {Futura implementacao de watchdog timer}
         datalogger.print("______________________________________________________________|");
         datalogger.close();											// Fecha o ponteiro de escrita do arquivo
         delay(500);													// Delay 0,5 segundo
      }
   }

   if (contador_de_interrupcao >= 2) {									// Caso contador_de_interrupcao for maior ou igual a 2:
      contador_de_interrupcao = 0;										// Zera contador_de_interrupcao
      Serial.println("Comecando a dormir");								// Printa no serial "Comecando a dormir"
      delay(500);														// Delay de 500 ms
      comecando_a_dormir();												// Chamada da função comecando_a_dormir
      delay(500);														// Delay de 500 ms
   }


} // fim loop


void acordando (){														// Função acordando
  Serial.println("acordando");											// Printa no serial "acordando"
  flag_acordando = true;												// Seta a flag_acordando em True
  //delayMicroseconds(2000); 											// delay para debug
}


void comecando_a_dormir(){												// Função de economia de energia
   digitalWrite(shield_SD, LOW);										// Desliga o shield_SD
   digitalWrite(shield_RTC, LOW);										// Desliga o shield_RTC
   set_sleep_mode(SLEEP_MODE_PWR_DOWN);									// Seta o sleep mode para desligamento quase total
   sleep_enable();														// Prepara para o desligamento dos registradores do processador
   attachInterrupt(0, acordando, FALLING);								// Cria interrupção externa responsável por fazer o circuito ligar
   sleep_mode();														// Desliga o processador
   sleep_disable();														// Quando o processador ligar ele executará esse comando e em seguida a interrupção
   detachInterrupt(0);													// Por fim, desabilita-se as interrupções
}


