#include <iom128v.h>
#include <avrdef.h>
#include <macros.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#define BEGIN 0
#define LVUP 1
#define WIN 2
#define LOSE 3
#define GAME 4

/*USART ���� �Լ�*/
void Init_USART0(void); //usart �ʱ�ȭ
void putch_USART0(char data);
char getch_USART0(void);

/*LCD ���� �Լ�*/
void delay_ms(unsigned int m);
void delay_us(unsigned int u);
void write_data(char d);
void write_instruction(char i);
void init_lcd(void);
void LCD_STR(char* str);
void LCD_char(char c);
void LCD_pos(unsigned char row, unsigned char col);
void erase_lcd(void);
void LCD_MODE(int mode);

/*���� ���� �Լ�*/
void life_LED(void); //LED ��� ǥ�� �Լ�
void main_game(void); //���� ����
void init(void); // ������/���� �й�/ ���� �¸� �̺�Ʈ �߻� �� �ʱ�ȭ
void lose(void); //���� �й� �� ����Ǵ� �Լ�
void win(void); //���� �¸� �� ����Ǵ� �Լ�
void LV_UP(void); //���� �� �� ����Ǵ� �Լ�

/*�ܺ� ���ͷ�Ʈ*/
void int0_isr(void); //�ܺ� ���ͷ�Ʈ 0 -> ���� ��� ���� ȭ�鿡�� ���� ����
void int1_isr(void); //�ܺ� ���ͷ�Ʈ 1 -> ���� ��� ��Ʈ ����
void int2_isr(void); //�ܺ� ���ͷ�Ʈ 2 -> ���� ��� ��Ʈ2 ����
void int3_isr(void); //�ܺ� ���ͷ�Ʈ 3 -> ���� ��� ��� ������Ŵ


/*���� ����*/
int hint_cnt = 0; //���ͷ�Ʈ 2����
int ans = 0; //����ڰ� �ܾ��� ���ڸ� �� �� ���߾����� ī��Ʈ�ϴ� ����
int level = 1; //���� - �� 6�ܰ�
int check = 0, die_cnt = 5, start = 1, go = 0;
//check : ����ڰ� ���ڸ� ������� �˻��ϴ� ����
//die_cnt : ��� �� 5��
//start : ó�� ������ HANGMAN���� �Ѵ�.
//go : �ܺ� ���ͷ�Ʈ 0���� ���Ǵ� ����
int second = 61; //�ʱ� �ð� (60��)
int ran = 0; //�ܾ��� �ε����� ��� �� ����
char data; //����ڰ� �Է��� ���ڸ� �����ϴ� ����
int cnt = 0; //�ܺ� ���ͷ�Ʈ 3���� ���Ǵ� ����
/*�ܾ�*/
char selected[7] = { 0 }; //���ӿ��� ����ڰ� �Է��� �ܾ�
int idx = 0; //selected �迭�� �ε���
int flag = 0; //����ڰ� �Է��� �ܾ����� üũ�ϴ� ����
char answer[] = { "_____" }; //���� _ _ _ _ _ ǥ��
char HANGMAN[] = { "HANGMAN" }; //ó�� ���� �ܾ�
char word[5][6] = { "study", "ocean", "earth", "house", "after" };
/*��Ʈ - �ܺ� ���ͷ�Ʈ 1 �߻� �� CLCD�� ���� ��Ʈ ����*/
char hint[5][6] = { "s___y", "o___n", "e___h", "h___e", "a___r" };
/*��Ʈ - �ܺ� ���ͷ�Ʈ 2 �߻� �� CLCD�� ������ �� ��Ʈ ����*/
char hint2[5][30] = {
    "prepare for test",
    "JEJU ISLAND",
    "Planet we live",
    "go after school",
    "reverse of before"
};
//�ܺ� ���ͷ�Ʈ 3 ���� ��� �ٽ� ���� ȭ������ ���ư�
void main(void) {
    /*���� ��� ǥ�� - G��Ʈ 0~4 LED 5��)*/
    DDRG = 0x1f;
    /*�¸� �� �Ҹ� ����*/
    DDRA = 0x01;
    /*CLCD ����*/
    DDRB = 0x07; //B��Ʈ 0����~2������ ������� ����
    DDRC = 0xff; //C��Ʈ ��� ���� ������� ����
    /*CLCD �ʱ�ȭ*/
    init_lcd();

    /*���ͷ�Ʈ ����*/
    DDRD = 0x00; //D��Ʈ ���ͷ�Ʈ ���
    EICRA = 0xAA; //INT0~3���(falling edge triggered)
    EIMSK = 0x0f; //�ܺ� ���ͷ�Ʈ 0~3 ���

    PORTG = 0x1f; //ó���� LED �� ����
    /*USART �ʱ�ȭ*/
    Init_USART0(); //USART �ʱ�ȭ
    SEI(); // �۷ι� ���ͷ�Ʈ ���
    LCD_MODE(BEGIN); //�ʱ� ȭ�� ǥ��(��� ����)
    while (1) {
        if (go == 1) { //���� ����
            LCD_MODE(GAME); //���� ȭ�� ǥ��
            while (1) {
                main_game(); //���� ���� �Լ� ȣ���Ͽ� ����
                if (go == 0) // go������ 0 (�¸� Ȥ�� �й�) �� ���
                    break; //�ݺ��� ����
            }
        }
    }
}
void main_game(void) {
    life_LED(); //��� ǥ��
    data = getch_USART0(); //data�� Ű����κ��� ���� �Է¹ޱ�
    putch_USART0(data); //�Է��� ���ڸ� puttyȭ�鿡 ǥ��
    if (start == 1) { //�� ó�� ������ �׻� HANGMAN ���� ����
        for (int i = 0; HANGMAN[i] != '\0'; i++) {
            if (HANGMAN[i] == data) { //����ڰ� �Է��� ���ڰ� �ܾ� �ȿ� �������
                LCD_pos(1, i);
                LCD_char(data); //�� ��ġ�� ǥ��
                check = 1;  //���� ������� üũ
                ans++; //���� ���� ���� ī��Ʈ
            }
        }
        if (check == 0) //�Է��� ���ڰ� ���� ���
            die_cnt--; //��� ����
        check = 0; //check ���� �ʱ�ȭ
        if (ans >= strlen(HANGMAN)) { //���� ���߸�
            start = 0; //�״������ʹ� ������ �ܾ� ����
            level++; //������
            LCD_MODE(LVUP); //���� �� ǥ��
            LCD_MODE(GAME); //���� ȭ�� �ٽ� ǥ��
            init(); //������ �ʱ�ȭ
            PORTG = 0x1f; //LED �� �ٽ� ����
        }
    }
    else { //�ι�° �������ʹ� ������ �ܾ�
        for (int i = 0; word[ran][i] != '\0'; i++) {
            if (word[ran][i] == data) {  //����ڰ� �Է��� ���ڰ� �ܾ� �ȿ� ���� ���
                for (int j = 0; j < 5; j++) { //����ڰ� �̹� �Է��ߴ� �������� Ȯ��
                    if (selected[j] == data) { //�Է��������
                        flag = 1; //flag = 1�� ����
                        break;
                    }
                }
                if (flag == 1) { //�Է��� ������ ���
                    check = 1; //check�� 1�� ����� �ǳ� �ڴ�
                    break;
                }
                selected[idx++] = data; //�Է¾��� ������ ��� selected �迭�� �߰�
                LCD_pos(1, i);
                LCD_char(data); //���� ��ġ�� ǥ��
                check = 1;  //���� ������� üũ
                ans++; //���� ���� ���� ī��Ʈ
            }
        }
        if (check == 0)//�Է��� ���ڰ� ���� ���
            die_cnt--; //��� ����
        check = 0;
        flag = 0;
        if (level == 6 && ans == strlen(word[ran])) {
            LCD_MODE(WIN); //��� ������ �� ������, LCD�� �¸� ȭ�� ǥ��
            PORTA = 0x01; //��Ƽ�� ���� �Ҹ� ���� ����
            delay_ms(300);
            PORTA = 0x00; //��Ƽ�� ���� �Ҹ��� �ٽ� ����
            ran = 0; //�ܾ� �ʱ�ȭ
            level = 1; //���� �ʱ�ȭ
            cnt = 0; //ī��Ʈ �ʱ�ȭ
            init(); //���� ���� �ʱ�ȭ
            start = 1; //HANGMAN �ܾ���� �ٽ� ������ ����
            go = 0; //go ������ 0���� �ٲ�� main�Լ��� �ݺ����� ������
            LCD_MODE(BEGIN);  //���� ���� ȭ������ ���ư���
        }
        else if (ans >= strlen(word[ran])) { //���� ���� ���
            for (int i = 0; i < 5; i++)
                selected[i] = 0; //�迭 �ʱ�ȭ
            idx = 0;  //selected �迭�� �ε����� ����� ���� �ʱ�ȭ
            level++; //������
            ran++; //���� �ܾ� ����
            LCD_MODE(LVUP); //������ ȭ�� ǥ��
            LCD_MODE(GAME); //�ٽ� ����ȭ������ ���ư�
            init(); //���� ���� �ʱ�ȭ
        }
    }
    if (die_cnt == 0) {
        PORTG = 0x00;
        LCD_MODE(LOSE); //�й� ȭ�� ǥ��
        delay_ms(300);
        ran = 0; //�ܾ� �ʱ�ȭ
        cnt = 0; //ī��Ʈ �ʱ�ȭ
        level = 1; //���� �ʱ�ȭ
        init(); //���� ���� �ʱ�ȭ
        start = 1; //HANGMAN �ܾ���� �ٽ� ������ ����
        go = 0; //go ������ 0���� �ٲ�� main�Լ��� �ݺ����� ������
        LCD_MODE(BEGIN); //���� ���� ȭ������ ���ư���
    }
}
void init(void) {
    die_cnt = 5; //��� �ʱ�ȭ
    check = 0;
    ans = 0; // ���� ���� �� �ʱ�ȭ
}
void life_LED(void) { //LED�� ��� ������ ǥ���ϴ� �Լ�
    switch (die_cnt) {
    case 0: //���� ����
        PORTG = 0x00;
        break;
    case 1: //��� 1��
        PORTG = 0x01;
        break;
    case 2: //��� 2��
        PORTG = 0x03;
        break;
    case 3: //��� 3��
        PORTG = 0x07;
        break;
    case 4: //��� 4��
        PORTG = 0x0f;
        break;
    case 5: //��� 5��
        PORTG = 0x1f;
        break;
    }
    /*��� �� ��ŭ LCD���� O�� ǥ��*/
    for (int i = 0; i < die_cnt; i++) {
        LCD_pos(0, 11 + i);
        LCD_char('O');
    }
    for (int i = 0; i < 5 - die_cnt; i++) {
        LCD_pos(0, 15 - i);
        LCD_char(' ');
    }
}

void Init_USART0(void) {
    //RXCIE0 : ���� �Ϸ� ���ͷ�Ʈ �㰡
    //UDRIE0 : ������ �������� �غ�Ϸ� ���ͷ�Ʈ �㰡
    //RXEN0 : ���� �㰡
    //TXEN0 : �۽� �㰡
    UCSR0B = (1 << RXCIE0) | (1 << UDRIE0) | (1 << RXEN0) | (1 << TXEN0);
    UCSR0A = 0x00;
    //���ڱ��� 8��Ʈ
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    UBRR0H = 0x00;
    UBRR0L = 0x67; //���巹��Ʈ 9600 ����
    //   SEI(); // SREG |= 0x80;
}
/*MCU -> putty ȭ������ �����ϱ�*/
void putch_USART0(char data) {
    while (!(UCSR0A & (1 << UDRE0))); //�����͸� �� �غ� �Ϸᰡ �Ǹ�
    UDR0 = data; //������ ����
}
/*putty -> MCU �� �Է��ϱ�*/
char getch_USART0(void) {
    while (!(UCSR0A & (1 << RXC0))); //������ �Ϸ�Ǹ�
    return UDR0; //��ȯ
}
/*�� ��Ȳ���� LCD�� ����ϴ� �Լ�*/
void LCD_MODE(int mode) {
    switch (mode) {
    case BEGIN: //���� ���� ȭ��
        erase_lcd();
        LCD_pos(0, 0);
        LCD_STR("  HANGMAN GAME");
        LCD_pos(1, 0);
        LCD_STR("  PRESS BUTTON");
        break;
    case LVUP:  //���� �� ȭ��
        erase_lcd();
        LCD_pos(0, 0);
        LCD_STR("Level UP!!!");
        delay_ms(200);
        break;
    case WIN:  //�¸� ȭ��
        erase_lcd();
        LCD_pos(0, 0);
        LCD_STR("YOU WIN!!!");
        PORTA = 0x01;
        break;
    case LOSE: //�й� ȭ��
        erase_lcd();
        LCD_pos(0, 0);
        LCD_STR("GAME OVER");
        LCD_pos(1, 0);
        LCD_STR("Try Again");
        break;
    case GAME: //���� ���� ȭ��
        erase_lcd();
        LCD_pos(0, 0);
        LCD_STR("Lv.");
        LCD_char(level + '0');
        LCD_pos(1, 0);
        if (start == 1)
            LCD_STR("_______");
        else
            LCD_STR(answer);
        break;
    }

}
/*��� ��ġ�� ���÷��� �� �� ���ϴ� �Լ�*/
/*instruction �Լ����� 0����, 1������ 'L' ���·� �Ѵ�*/
void write_instruction(char i) {
    PORTB = 0x04; //instruction ���� RS = 0���� �����Ѵ�.
    delay_us(10);
    PORTC = i; //��ġ �� ����
    delay_us(10);
    PORTB = 0x00; //2�� ���� Enable 1->0(�ϰ� ����)�� ��
    delay_us(100);
}

/*� ���ڸ� ����� �� ���ϴ� �Լ�*/
/*write �Լ����� 0������ 'H', 1������ 'L' ���·� �Ѵ�*/
void write_data(char d) {
    PORTB = 0x05; //write ���� RS = 1 �� �����Ѵ�.
    delay_us(100);
    PORTC = d; //����� ���� ����
    PORTB = 0x01; //2�� ���� Enable �� 1->0(�ϰ� ����)�� ��
    delay_us(100);
}
/*clcd �ʱ�ȭ �Լ�*/
void init_lcd(void) {
    delay_ms(10);
    write_instruction(0x30); //��� ����
    delay_ms(25);
    write_instruction(0x30); //��� ����
    delay_ms(5);
    write_instruction(0x30); //��� ����
    delay_ms(5);
    write_instruction(0x3c); //��� ����
    delay_ms(5);
    write_instruction(0x08); //ǥ�� ��,����
    delay_ms(5);
    write_instruction(0x01); //ǥ�� Ŭ����
    delay_ms(5);
    write_instruction(0x06); //��Ʈ����� ����
    delay_ms(5);
    write_instruction(0x0c); //ǥ�� ��,���� ����
    delay_ms(15);
}
/*������ �Լ�*/
void delay_us(unsigned int u) {
    unsigned int i, j;
    for (i = 0; i < u; i++) {
        for (j = 0; j < 2; j++);
    }
}
/*������ �Լ�*/
void delay_ms(unsigned int m) {
    unsigned int i, j;
    for (i = 0; i < m; i++) {
        for (j = 0; j < 2100; j++)
            ;
    }
}
/*���ڿ��� ����ϴ� �Լ�*/
void LCD_STR(char* str) {
    for (int i = 0; str[i] != 0; i++) {
        write_data(str[i]);
    }
}
/*���� �ϳ��� ����ϴ� �Լ�*/
void LCD_char(char c) {
    write_data(c);
    delay_ms(5);
}
/*��,���� �Է��ϸ� �� ��ġ�� CLCD�� �ּҰ��� �������ִ� �Լ�*/
void LCD_pos(unsigned char row, unsigned char col) {
    write_instruction(0x80 | (row * 0x40 + col)); //��(row)�� ��(col)
}
/*LCD�� ��� ����� �Լ�*/
void erase_lcd(void) {
    LCD_pos(0, 0);
    LCD_STR("                   ");
    LCD_pos(1, 0);
    LCD_STR("                   ");
}

//���ͷ�Ʈ 0 - ���� ����
#pragma interrupt_handler int0_isr:iv_INT0
void int0_isr(void) {
    go = 1; //������ ���� ����
}
//���ͷ�Ʈ 1 - ���� ��Ʈ ����
#pragma interrupt_handler int1_isr:iv_INT1
void int1_isr(void) {
    if (start == 0) { //�ι�° �������� ��Ʈ ��� ����
        ans += 2; //��Ʈ ��� �� ���� ���� 2�� ����
        /*LCD�� ǥ��*/
        LCD_pos(1, 0);
        LCD_char(hint[ran][0]);
        LCD_pos(1, 4);
        LCD_char(hint[ran][4]);
        for (int i = 0; i < 5; i++) { //���࿡ ��Ʈ�� ����ڰ� �̹� ���� ������ ��� ���� ���ڼ����� ������
            if (selected[i] == hint[ran][0] || selected[i] == hint[ran][4])
                ans--;
        }
        selected[idx++] = hint[ran][0];
        selected[idx++] = hint[ran][4];
    }
}
//���ͷ�Ʈ 2 - ���� ��Ʈ ����
#pragma interrupt_handler int2_isr:iv_INT2
void int2_isr(void) {
    if (start == 0) { //�ι�° �ܾ���� ��Ʈ ���� ����
        hint_cnt++; //���ͷ�Ʈ �߻� �� cnt ����
        if (hint_cnt == 1) { //cnt�� 1�̸� ��Ʈ ���� �� LCD�� ǥ��
            erase_lcd();
            LCD_pos(0, 0);
            LCD_STR(hint2[ran]);
        }
        if (hint_cnt == 2) { //�ѹ� �� ���� ���
            hint_cnt = 0; //cnt�� �ʱ�ȭ�ǰ�
            erase_lcd();  //���� ���� ȭ������ ���ƿ�
            LCD_MODE(GAME);
            life_LED(); //��� ǥ��
            /*���� ȭ�� �ٽ� ǥ��*/
            for (int i = 0; i < 5; i++) {
                if (selected[i] != 0) {
                    for (int j = 0; j < 5; j++) {
                        if (word[ran][j] == selected[i]) {
                            LCD_pos(1, j);
                            LCD_char(selected[i]);
                        }
                    }
                }
            }
        }
    }
}
//���ͷ�Ʈ 3 - ��� ���� ���� (3�������� ����)
#pragma interrupt_handler int3_isr:iv_INT3
void int3_isr(void) {
    if(cnt < 3){
        if(die_cnt < 5)
            die_cnt++;
        life_LED(); //��� ������ ǥ���ϴ� �Լ�
    }
    cnt++;
}
#pragma interrupt_handler uart0_tx_isr:iv_USART0_TXC
void uart0_tx_isr(void) {
}

#pragma interrupt_handler uart0_rx_isr:iv_USART0_RXC
void uart0_rx_isr(void) {
}
