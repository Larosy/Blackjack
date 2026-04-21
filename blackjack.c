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

// --- OYUN DE???KENLER? VE PWM ---
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

// --- ANA OYUN DÖNGÜSÜ ---
void Penalty() {
    lives--; // Can kayb? [cite: 14, 46]
    LATD = (LATD & 0xF8) | ((1 << lives) - 1); // Can LED'lerini güncelle 
    BUZZER = 1; __delay_ms(500); BUZZER = 0; // Sesli uyar?
    servo_pos = 5 + (rand() % 20); 
    __delay_ms(1000);
}

void main() {
    TRISB = 0xF0; INTCON2bits.RBPU = 0; 
    TRISCbits.TRISC1 = 0; TRISCbits.TRISC2 = 0; TRISD &= 0xF8;
    T1CON = 0x01; PIE1bits.TMR1IE = 1; INTCONbits.GIE = 1; INTCONbits.PEIE = 1;
    
    Lcd_Init();
    srand(500); // Ba?lang?ç seed

    while(1) {
        if(lives == 0) {
            Lcd_Cmd(0x01); Lcd_Print("GAME OVER!");
            while(Get_Key() != 'C'); //Reset
            lives = 3; p_score = 0;
        }

        Lcd_Cmd(0x01); Lcd_Print("SKOR:");
        char s[5]; sprintf(s, "%d", p_score); Lcd_Print(s);
        Lcd_Cmd(0xC0); Lcd_Print("7:HIT 8:STAND");

        char k = 0;
        while(!(k = Get_Key())); __delay_ms(300);

        if(k == '7') { // Kart çek 
            p_score += (rand() % 10) + 1;
            if(p_score > 21) {
                Lcd_Cmd(0x01); Lcd_Print("BUST! 21 GECTI");
                Penalty(); p_score = 0;
            }
        } else if(k == '8') { // Dur 
            d_score = 15 + (rand() % 7); // Dealer AI
            if(d_score > 21 || p_score > d_score) {
                Lcd_Cmd(0x01); Lcd_Print("KAZANDIN!");
            } else {
                Lcd_Cmd(0x01); Lcd_Print("KASA KAZANDI");
                Penalty();
            }
            p_score = 0; __delay_ms(2000);
        }
    }
}