#include <xc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// --- KONF?GÜRASYON AYARLARI ---
#pragma config FOSC = HS, WDT = OFF, LVP = OFF, PBADEN = OFF

#define _XTAL_FREQ 20000000

// --- PIN TANIMLAMALARI ---
#define RS LATEbits.LATE0
#define EN LATEbits.LATE1
#define LCD_PORT LATD   // D4-D7 pinleri RD4-RD7'ye 
#define SERVO_PIN LATCbits.LATC2
#define BUZZER LATCbits.LATC1

/*
// --- LCD SÜRÜCÜSÜ (4-BIT MODU) ---
void Lcd_Cmd(unsigned char cmd) {
    RS = 0;
    LCD_PORT = (LCD_PORT & 0x0F) | (cmd & 0xF0);
    EN = 1; __delay_us(40); EN = 0;
    LCD_PORT = (LCD_PORT & 0x0F) | (cmd << 4);
    EN = 1; __delay_us(40); EN = 0;
    __delay_ms(2);
}

void Lcd_Write(unsigned char data) {
    RS = 1;
    LCD_PORT = (LCD_PORT & 0x0F) | (data & 0xF0);
    EN = 1; __delay_us(40); EN = 0;
    LCD_PORT = (LCD_PORT & 0x0F) | (data << 4);
    EN = 1; __delay_us(40); EN = 0;
    __delay_ms(2);
}

void Lcd_Print(const char* str) {
    while(*str) Lcd_Write(*str++);
}

void Lcd_Init() {
    TRISEbits.TRISE0 = 0; TRISEbits.TRISE1 = 0;
    TRISD &= 0x0F; // RD4-RD7 Ç?kt?s?
    __delay_ms(20);
    Lcd_Cmd(0x02); Lcd_Cmd(0x28); Lcd_Cmd(0x0C); Lcd_Cmd(0x01);
}
*/


// --- UART (NEXTATION) ---
void UART_Init(){
    TRISCbits.TRISC6 = 0; //TX
    TRISCbits.TRISC7 = 1; //RX
    SPBRG = 129;          // 9600 Baud @ 20MHz
    TXSTA = 0x24;         // TX aktif, High speed
    RCSTA = 0x90;         // Seri port aktif
}

void Next_Send(const char* cmd){
    while(*cmd){
        while(!PIR1bits.TXIF);
        TXREG = *cmd++;
    }
    
    for(int i = 0; i < 3; i++){
        while(!PIR1bits.TXIF);
        TXREG = 0xFF;
    }
}

// --- KEYPAD TARAMA (4x4) ---
char Get_Key() {
    char keys[4][4] = 
    {{'7','8','9','/'},
    {'4','5','6','*'},
    {'1','2','3','-'},
    {'C','0','=','+'}};
    for(int r=0; r<4; r++) {
        LATB = ~(1 << r); 
        __delay_us(10);
        if(!PORTBbits.RB4) return keys[r][0];
        if(!PORTBbits.RB5) return keys[r][1];
        if(!PORTBbits.RB6) return keys[r][2];
        if(!PORTBbits.RB7) return keys[r][3];
    }
    return 0;
}

// --- DE???KENLER VE PWM ---
volatile uint8_t servo_pos = 15;
uint8_t p_score = 0, d_score = 0, lives = 3;

void __interrupt() ISR() {
    if(PIR1bits.TMR1IF) { // 20ms Servo Sinyali
        static uint16_t c = 0;
        SERVO_PIN = (c < servo_pos);
        if(++c >= 200) c = 0;
        TMR1H = 0xFF; TMR1L = 0x38; PIR1bits.TMR1IF = 0;
    }
}

// --- KART ÇEKME VE EKRANA GÖNDERME ---
uint8_t Draw_Card_To_Screen(uint8_t target_p_index) {
    // 1 ile 13 aras?nda rastgele bir kart çek
    uint8_t raw_card = (rand() % 13) + 1; 
    // Nextion'daki resim ID'sini hesapla
    uint8_t card_pic_id = raw_card + 10;  
    char cmd[32];
    sprintf(cmd, "p%d.pic=%d", target_p_index, card_pic_id);
    Next_Send(cmd);
    // E?er çekilen kart Vale(11), K?z(12) veya Papaz(13) ise oyundaki puan de?eri 10'dur.
    if (raw_card > 10) {
        return 10; 
    }
    // E?er kart As(1) ise ve oyuncunun puan? 11 veya daha azsa As'? 11 sayma
    else if (raw_card == 1 && p_score <= 11) {
        return 11;
    }
    return raw_card; 
}

// --- ANA OYUN DÖNGÜSÜ ---
void Penalty() {
    lives--; // Can kayb?
    LATD = (LATD & 0xF8) | ((1 << lives) - 1); // Can LED'lerini güncelle 
    BUZZER = 1; __delay_ms(500); BUZZER = 0; // Sesli uyar?
    servo_pos = 5 + (rand() % 20); 
    
    char l_cmd[16];
    sprintf(l_cmd, "n_lives.val=%d", lives);
    Next_Send(l_cmd);
    __delay_ms(1000);
}

// --- YEN? ROUND BA?LANGICI ---
void Reset_Round() {
    p_score = 0;
    d_score = 0;
    Next_Send("t_player.txt=\"0\"");
    Next_Send("t_dealer.txt=\"0\"");
    Next_Send("t_result.txt=\"\"");
}

// --- ANA DÖNGÜ ---
void main() {
    TRISB = 0xF0; INTCON2bits.RBPU = 0; // Keypad
    TRISCbits.TRISC1 = 0; TRISCbits.TRISC2 = 0; TRISD &= 0xF8;
    UART_Init();
    T1CON = 0x01; PIE1bits.TMR1IE = 1; INTCONbits.GIE = 1; INTCONbits.PEIE = 1;
    
    srand(500); // Rastgelelik için seed
    __delay_ms(500);
    Reset_Round();

    while(1) {
        // Can biterse Game Over durumuna dü?
        if(lives == 0) {
            Next_Send("t_result.txt=\"GAME OVER!\"");
            while(Get_Key() != 'C'); // Temizleme tu?u (C) bekler
            lives = 3;
            LATD = 0x07;
            Reset_Round();
        }

        char k = Get_Key();
        if(k == '7') {
            uint8_t raw_card = (rand() % 13) + 1; // 1-13 aras? (As, J, Q, K dahil)
            uint8_t points = (raw_card > 10) ? 10 : raw_card; // J, Q, K = 10 puan
            p_score += points;

            // t_player alan?na yeni skoru yazd?r
            char cmd[32];
            sprintf(cmd, "t_player.txt=\"%d\"", p_score);
            Next_Send(cmd);
            
            if(p_score > 21) {
                Next_Send("t_result.txt=\"BUST! 21 GECTI\"");
                Penalty();
                __delay_ms(2000);
                Reset_Round();
            }
            __delay_ms(300); // Tu? s?çrama önleyici 
        } 
        else if(k == '8') { 
            while(d_score < 17) {
                uint8_t d_raw = (rand() % 13) + 1;
                d_score += (d_raw > 10) ? 10 : d_raw;
            }

            // t_dealer alan?na kasan?n skorunu yazd?r
            char cmd[32];
            sprintf(cmd, "t_dealer.txt=\"%d\"", d_score);
            Next_Send(cmd);

            // Sonuç Kar??la?t?rma
            if(d_score > 21 || p_score > d_score) {
                Next_Send("t_result.txt=\"KAZANDIN!\"");
            } else {
                Next_Send("t_result.txt=\"KAYBETTIN!\"");
                Penalty();
            }
            __delay_ms(3000); // Sonucu ekranda görme süresi
            Reset_Round();
        }
    }
}