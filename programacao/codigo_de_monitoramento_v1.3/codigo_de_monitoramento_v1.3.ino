
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
 * Consiste na comunicação SPI com o módulo cartão SD, na comuni-
 * cação I2C com o módulo DS3231. Além de possuir sua saída, ori-
 * ginalmente o pino 5 ligado a um resistor que é ligado à um LED
 * em current source. Ademais, os dois botões estão ligados em 
 * pull up. Originalmente, nos digitais 6 e 7.
 * Para mais informações, consute os esquemáticos na raiz de todo
 * o projeto, seguindo o link do GitHub, apresentado no cabeçalho 
 * deste projeto.
 * 
*/


//-----------------------------------------------------------------
// §§§§ Declaração das bibliotecas §§§§  //
#include <avr/sleep.h>
#include <DS3231.h>
#include <SPI.h>
#include <SD.h>


//-----------------------------------------------------------------
// §§§§ Definições de Hardware §§§§  //
#define entrada1 6    
#define entrada2 5    
#define saida    7    
#define interrupcao 2 
#define chip_select 4 
#define shield_RTC 8  
#define shield_SD 9   

//-----------------------------------------------------------------
// §§§§ Declaração das variáveis globais §§§§  //
bool estado1 = false;
bool estado2 = false;
bool flag1   = false;
bool flag2   = false;

volatile byte flag_acordando = false;

unsigned int contador_flag = 0;
unsigned int minuto_antigo = 0;
unsigned int minuto_atual = 0;
unsigned int contador_de_interrupcao = 0;
unsigned int contador_de_pulsos_botao_1 = 0;
unsigned int contador_de_pulsos_botao_2 = 0;
unsigned int contador_de_pulsos = 0;
unsigned long tempo_atual = 0;
unsigned long tempo_de_delay = 0;
byte ultimo_botao = 0;
int contador_de_debug = 0;

//-----------------------------------------------------------------
// §§§§ Declaração dos objetos §§§§  //
DS3231  rtc(SDA, SCL);
Time  t;
File datalogger;


//-----------------------------------------------------------------
// §§§§ Declaração de funcoes adicionais §§§§  //
void comecando_a_dormir();
void acordando();


//-----------------------------------------------------------------
// §§§§ Void Setup §§§§  //
void setup() {
  // delay(200);
   pinMode(entrada1, INPUT);
   pinMode(entrada2, INPUT);
   pinMode(interrupcao, INPUT);
   pinMode(saida, OUTPUT);
   pinMode(shield_RTC, OUTPUT);
   pinMode(shield_SD, OUTPUT);
   pinMode(3, OUTPUT);
   Serial.begin(115200);
   int i = 150;
   rtc.begin();
   while (!Serial) {;}
   
   digitalWrite(shield_SD, HIGH);
   delay(1);
   if (!SD.begin(chip_select)) {
     Serial.println("Erro ao ler cartao de memoria");
     return;
  }
   //rtc.setDOW(FRIDAY);          // Selecione o dia em ingles; Ex: (SUNDAY) - Domingo
   //rtc.setTime(14, 30, 10);     // Selecione a hora; Ex: (14, 30, 10)  -- 14 horas, 30 minutos e 10 segundos
   //rtc.setDate(20, 7, 2018);    // Selecione a data; Ex: (20, 7, 2018) -- Dia 20 do mes 7 de 2018.
   
   while(1){
      digitalWrite(saida, HIGH);
      delay(i);
      i = i-10;
      digitalWrite(saida, LOW);
      delay(i);
      if (i < 50) {i = i+9;}
      if (i == 9) {break;}
   }
   digitalWrite(saida, HIGH);
   delay(500);
   digitalWrite(saida, LOW);
   attachInterrupt(0, acordando, RISING);
   //detachInterrupt(0);
}



//-----------------------------------------------------------------
// §§§§ Void Loop §§§§  //
void loop() {
   digitalWrite(shield_SD, HIGH);
   digitalWrite(shield_RTC, HIGH);

   t = rtc.getTime();  
   minuto_atual = t.min;
   
   if (flag_acordando){
      tempo_de_delay = millis();
      tempo_atual = tempo_de_delay;
      flag_acordando = false;
   }
   
   if(tempo_de_delay != 0){
      tempo_de_delay = millis();
   }
   
   if (contador_de_debug == 20000){
      Serial.print("Tempo_atual: ");
      Serial.print(tempo_atual);
      Serial.print("      ");
      Serial.print("Tempo_de_delay: ");
      Serial.print(tempo_de_delay);
      Serial.print("      ");
      Serial.print("Diferença de tempo: ");
      Serial.print(tempo_de_delay - tempo_atual);
      Serial.print("      ");
      Serial.print(minuto_antigo);
      Serial.print(" ");
      Serial.print(minuto_atual);
      Serial.println("      ");
      contador_de_debug = 0;
   }
   //contador_de_debug++;
   
   if((tempo_de_delay - tempo_atual) < 50000){
      minuto_antigo = minuto_atual;
   }
   
   if ( minuto_antigo != minuto_atual ){
      flag_acordando = false;
      tempo_atual = tempo_de_delay;
      minuto_antigo = minuto_atual;
      
      Serial.println(" ");
      Serial.println(" ");
      Serial.println(" Atualizando datalogger ");
      Serial.print("O total de pulsos nesse minuto foram de: ");
      Serial.println(contador_de_pulsos);
      Serial.print("O total de pulsos do botao 1 foi de: ");
      Serial.println(contador_de_pulsos_botao_1);
      Serial.print("O total de pulsos do botao 2 foi de: ");
      Serial.println(contador_de_pulsos_botao_2);
      
      
      if (contador_de_pulsos == 0) {
         contador_de_interrupcao++;
      } else {
         contador_de_interrupcao = 0;
      }
      
      datalogger = SD.open("ValorT.svc", FILE_WRITE);
      if ( datalogger ) {
         Serial.println("___________________________.DATALOGGER ATT.______________________________");
         Serial.print("DATA: ");
         Serial.print(rtc.getDateStr());
         Serial.print("  HORA: ");
         Serial.print(rtc.getTimeStr());
         Serial.print("   ");
         Serial.println();
//         datalogger.println("    Data,    |    Hora,    |   Contagem,   |   Temperatura,   |");
         datalogger.print(rtc.getDateStr());
         datalogger.print(",");
         datalogger.print(rtc.getTimeStr());
         datalogger.print(",");
         if(contador_de_pulsos < 10) {datalogger.print("0");}
         datalogger.print(contador_de_pulsos);
         if(contador_de_pulsos_botao_1 < 10) {datalogger.print("0");}
         datalogger.print(contador_de_pulsos_botao_1);
         if(contador_de_pulsos_botao_2 < 10) {datalogger.print("0");}
         datalogger.print(contador_de_pulsos_botao_2);
         datalogger.print(",");
         datalogger.print(rtc.getTemp());
         datalogger.println(",");
         datalogger.close();
      } else {
         Serial.println("Erro ao abrir datalogger");
      }
      contador_de_pulsos = 0;
      contador_de_pulsos_botao_1 = 0;
      contador_de_pulsos_botao_2 = 0;
      Serial.println("Atualizado");
   
   }
   
   estado1 = digitalRead(entrada1);
   estado2 = digitalRead(entrada2);

   if ( estado1 == LOW ) {
      flag1 = true;
   }

   if ( estado2 == LOW ) {
      flag2 = true;
   }

   if ( (estado1 == HIGH) && (flag1 == true) && (ultimo_botao != 1) ){    // botão 1 trocou de estado -- botao apertado
     // while(digitalRead(entrada1) == HIGH){;}
      Serial.println("botao 1 apertado");
      digitalWrite(saida, LOW);
      contador_de_pulsos_botao_1++;
      contador_de_pulsos ++;
      digitalWrite(saida, HIGH);
      delay(20);
      digitalWrite(saida, LOW);
      delay(20);
      flag1 = false;
      ultimo_botao = 1;
   }

   if ( (estado2 == HIGH) && (flag2 == true) && (ultimo_botao != 2) ){    // botão 2 trocou de estado -- botao apertado
      Serial.println("botao 2 apertado");
      digitalWrite(saida, HIGH);
      contador_de_pulsos_botao_2 ++;
      contador_de_pulsos ++;
      digitalWrite(saida, HIGH);
      delay(20);
      digitalWrite(saida, LOW);
      delay(20);
      flag2 = false;
      ultimo_botao = 2;
   }
   
   if ((estado1 == HIGH) && (estado2 == HIGH) && (flag1 == true) && (flag2 == true)) { // Erro, pois não se pode ter os dois botões ou as duas flags setadas ao mesmo tempo.
      Serial.println("Erro lógico, Duas entradas em HIGH");
      datalogger = SD.open ("Valores.svc, FILE_WRITE");
      if (datalogger) {
         datalogger.print("______________________________________________________________|");
         datalogger.print(rtc.getDateStr());
         datalogger.print(",  |  ");
         datalogger.print(rtc.getTimeStr());
         datalogger.print(",  |      ");
         if(contador_de_pulsos < 10) {datalogger.print("0");}
         datalogger.print(contador_de_pulsos);
         datalogger.print(",       |      ");
         datalogger.print(rtc.getTemp());
         datalogger.println(",      |");
         datalogger.print("Erro lógico, Duas entradas em HIGH");
         datalogger.print("______________________________________________________________|");
         datalogger.close();
         delay(500);
      }
   }

   if (contador_de_interrupcao >= 2) {
      contador_de_interrupcao = 0;
      Serial.println("Comecando a dormir");
      delay(500);
      comecando_a_dormir();
      delay(500);
   }


} // fim loop


void acordando (){
  Serial.println("acordando");
  flag_acordando = true;
  //delayMicroseconds(2000); // delay para debug
}


void comecando_a_dormir(){
   digitalWrite(shield_SD, LOW);
   digitalWrite(shield_RTC, LOW);
   set_sleep_mode(SLEEP_MODE_PWR_DOWN);
   sleep_enable();
   attachInterrupt(0, acordando, FALLING);
   sleep_mode();
   sleep_disable();
   detachInterrupt(0);
}


