#include <iom128v.h>
#include <avrdef.h>
/*��� ����*/
#define CLOCK 1 //�Ϲ� ������ �ð���
#define GMT 2  //���� ǥ�ؽ� ���
#define SW 3  //�����ġ ���
#define ALARM 4 //�˶��� �︱��� ���
/*�Լ� ���� ����*/
void delay_ms(unsigned int m);
void delay_us(unsigned int u);
void write_data(char d);
void write_instruction(char i);
void init_lcd(void);
void LCD_STR(char* str);
void LCD_char(char c);
void LCD_pos(unsigned char row, unsigned char col);
void ModeSet(int m);

/*���ͷ�Ʈ*/
void int0_isr(void);
void int1_isr(void);
void int2_isr(void);
void int3_isr(void);
void int4_isr(void);
void int5_isr(void);
void int6_isr(void);
/*Ÿ�̸�*/
void timer0_ovf_isr(void); //Ÿ�̸�0
void timer2_ovf_isr(void); //Ÿ�̸�2
int alarm = 1; //�˶� ����
unsigned cnt = 0; //Ÿ�̸� 0���� ī��Ʈ�ϴ� ����
unsigned cnt2 = 0; //Ÿ�̸� 2���� ī��Ʈ�ϴ� ����
/*�ð� ����*/
int hour = 16, min = 50, sec = 0; //�ð� ��,��,�ʸ� ������ ����
int c_sec1, c_min1, c_hour1; // �ð� 10���ڸ� ����
int c_sec2, c_min2, c_hour2; // �ð� 1���ڸ� ����
/*���� ����*/
int day = 0;
/*���� �ð� ����*/
int z_hour; //���� �ð��� �ø� ������ ����
int zulu = 1; //���� �ð� ��� üũ ����
int z_hour1, z_hour2, zulu_day = 0; //���� �ð� �� 10���ڸ�, 1���ڸ�, ������ ������ ����

/*�����ġ ����*/
int sw_hour = 0, sw_min = 0, sw_sec = 0, point1 = 0, point2 = 0, sw_cnt = 0;
int s_hour1 = 0, s_hour2 = 0, s_min1 = 0, s_min2 = 0, s_sec1 = 0, s_sec2 = 0;

/*������ ���� �迭*/
char* week[] = {
  "[Sunday]        ",
  "[Monday]        ",
  "[Tuesday]       ",
  "[Wednesday]     ",
  "[Thursday]      ",
  "[Friday]        ",
  "[Saturday]      "
};
/* ���� ǥ�ؽ��� ������ ���� �迭 */
char* z_week[] = {
  "[Sunday]     GMT",
  "[Monday]     GMT",
  "[Tuesday]    GMT",
  "[Wednesday]  GMT",
  "[Thursday]   GMT",
  "[Friday]     GMT",
  "[Saturday]   GMT"
};

void main(void) {
    /*CLCD ����*/
    DDRB = 0x07; //B��Ʈ 0����~2������ ������� ����
    DDRC = 0xff; //C��Ʈ ��� ���� ������� ����
    init_lcd(); //CLCD �ʱ�ȭ
    /*������ ������ LED ����*/
    DDRA = 0x01; // 0������ ������� ���, 1������ �Է����� ���
    /*���ͷ�Ʈ �ʱ�ȭ(D,E��Ʈ�� �Է����� ���)*/
    DDRD = 0x00;
    DDRE = 0x00;
    EICRA = 0xAA; //INT0,1,2,3 ���(falling edge triggered)
    EICRB = 0x2A; //INT4,5,6 ���(falling edge triggered)

    EIMSK = 0x7F; //INT0,1,2,3,4,5,6 �ο��̺�

    /*Ÿ�̸� ����*/
    TCCR0 = 0x04; // �븻���, ���ֺ� 64
    TCCR2 = 0x0b; //ctc ���, ���ֺ� 64
    TIMSK = 0x01; //�����÷ο� ���ͷ�Ʈ ����(�Ϲ� �ð�� �����÷ο� ��� ���)
    SEI(); //���·������� I��Ʈ�� 1�� ����(Global Interrupt enable)

    while (1) {
        /*
            8�ÿ� �˶� �︲(���� �Ͼ�� �ð��Դϴ�)
            �˶��� ������ alarm = -1�� �ǰ�, �Ϸ簡 ������ �ٽ� alarm = 0
            �� �Ǿ� ������ �ٽ� �˶��� �︮�� �ȴ�.
        */
        if (alarm == 0 && hour >= 8 && min >= 0) {
            PORTA = 0x01; //A��Ʈ 0���ɿ� HIGH ���
            ModeSet(ALARM); //�˶� ���
        }
        else {
            c_hour1 = hour / 10; //�� 10���ڸ�
            c_hour2 = hour % 10; //�� 1���ڸ�
            c_min1 = min / 10;   //�� 10���ڸ�
            c_min2 = min % 10;   //�� 1���ڸ�
            c_sec1 = sec / 10;   //�� 10���ڸ�
            c_sec2 = sec % 10;   //�� 1���ڸ�
            z_hour = hour - 9; //���� ǥ�� �ð��� (�ѱ��ð� - 9�ð�)
            if (z_hour < 0) { //���� ǥ�� �ð��� ������ �� ���
                z_hour += 24; //24�ø� ���ؼ� ����� �������
                zulu_day = (zulu_day - 1 + 7) % 7; //������ �Ϸ� ������ ����
            }
            /*���� �ð� �� 10�� �ڸ��� 1�� �ڸ�*/
            z_hour1 = z_hour / 10;
            z_hour2 = z_hour % 10;
            /*��ž��ġ ��,��,��*/
            s_hour1 = sw_hour / 10; //��ž��ġ �� 10���ڸ�
            s_hour2 = sw_hour % 10; //��ž��ġ �� 1���ڸ�
            s_min1 = sw_min / 10; //��ž��ġ �� 10���ڸ�
            s_min2 = sw_min % 10; //��ž��ġ �� 1���ڸ�
            s_sec1 = sw_sec / 10; //��ž��ġ �� 10���ڸ�
            s_sec2 = sw_sec % 10; //��ž��ġ �� 10���ڸ�

            if (zulu % 2 != 0 && sw_cnt == 0)//�ѱ��ð� ����� ���
                ModeSet(CLOCK);
            //����ð� ����� ���(�ѱ��ð�� �����ϱ� ���� GMT(Greenwich Mean Time) ǥ�ñ�� �߰�
            else if (zulu % 2 == 0 && sw_cnt == 0)
                ModeSet(GMT);
            else //�����ġ ����� ���
                ModeSet(SW);
            //���� ī��Ʈ
            day %= 7; //day�� 0~6�� ���� ������
            zulu_day = day; //���� ǥ�ؽð��� ������ �����Ѵ�.
        }
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
/*mode�� �����Ͽ� CLCD�� ����ϴ� �Լ�*/
void ModeSet(int mode) {
    switch (mode) {
    case 1: //�Ϲ� �ð� ���
        LCD_pos(0, 0); //0�� 0������ ���
        LCD_STR(week[day]); //���� ���
        if (hour < 12) { //����
            LCD_pos(1, 0);//�� �� �� ������ ���
            LCD_STR("AM"); //am ���
            LCD_char(' ');
            LCD_char(c_hour1 + '0');
            LCD_char(c_hour2 + '0');
            LCD_char(':');
            LCD_char(c_min1 + '0');
            LCD_char(c_min2 + '0');
            LCD_char(':');
            LCD_char(c_sec1 + '0');
            LCD_char(c_sec2 + '0');
            LCD_STR("       ");
        }
        else if (hour >= 12) { //����
            LCD_pos(1, 0);//�� �� �� ������ ���
            LCD_STR("PM"); //pm ���
            LCD_char(' ');
            LCD_char(c_hour1 + '0');
            LCD_char(c_hour2 + '0');
            LCD_char(':');
            LCD_char(c_min1 + '0');
            LCD_char(c_min2 + '0');
            LCD_char(':');
            LCD_char(c_sec1 + '0');
            LCD_char(c_sec2 + '0');
            LCD_STR("       ");
        }
        break;
    case 2: //���� ǥ�ؽ� ���
        LCD_pos(0, 0);
        LCD_STR(z_week[zulu_day]); //���� ���
        if (z_hour < 12) {
            LCD_pos(1, 0);//�� �� �� ������ ���
            LCD_STR("AM"); //am ���
            LCD_char(' ');
            LCD_char(z_hour1 + '0');
            LCD_char(z_hour2 + '0');
            LCD_char(':');
            LCD_char(c_min1 + '0');
            LCD_char(c_min2 + '0');
            LCD_char(':');
            LCD_char(c_sec1 + '0');
            LCD_char(c_sec2 + '0');
            LCD_STR("       ");
            LCD_pos(0, 14); //���� ���
            LCD_STR("GMT"); //GMT ǥ�� ���
        }
        else if (z_hour >= 12) {
            LCD_pos(1, 0);//�� �� �� ������ ���
            LCD_STR("PM"); //pm ���
            LCD_char(' ');
            LCD_char(z_hour1 + '0');
            LCD_char(z_hour2 + '0');
            LCD_char(':');
            LCD_char(c_min1 + '0');
            LCD_char(c_min2 + '0');
            LCD_char(':');
            LCD_char(c_sec1 + '0');
            LCD_char(c_sec2 + '0');
            LCD_STR("       ");
            LCD_pos(0, 14); //���� ��ܿ�
            LCD_STR("GMT"); //GMT ǥ�� ���
        }
        break;
    case 3: //�����ġ ���
        LCD_pos(0, 0);
        LCD_STR("Stop Watch      ");
        LCD_pos(1, 0);//�� �� �� ������ ���
        LCD_char(s_hour1 + '0');
        LCD_char(s_hour2 + '0');
        LCD_char('h');
        LCD_char(s_min1 + '0');
        LCD_char(s_min2 + '0');
        LCD_char('m');
        LCD_char(s_sec1 + '0');
        LCD_char(s_sec2 + '0');
        LCD_char('.');
        LCD_char(point1 + '0');
        LCD_char(point2 + '0');
        LCD_char('s');
        break;
    case 4: //�˶��� ����� ��
        LCD_pos(0, 0);
        LCD_STR("WAKE UP!!!       ");
        LCD_pos(1, 0);
        LCD_STR("WAKE UP!!!       ");
        break;
    }
}
#pragma interrupt_handler timer0_ovf_isr:iv_TIM0_OVF
void timer0_ovf_isr(void) { //�Ϲ� �ð� ���
    TCNT0 = 6;//���ֺ�64�� ����-> �ֱ�=0.000004�� ->250000ȸ ī��Ʈ�� ��� 1��
    cnt++; //250�� ī��Ʈ �� ������ �����÷ο� �߻� -> 4�� �߻��ϸ� 1��
    if (cnt >= 1000) {
        sec++; //�� ����
        cnt = 0;
    }
    if (sec == 60) {
        min++; //�� ����
        sec = 0;
    }
    if (min == 60) {
        hour++; //�� ����
        min = 0;
    }
    if (hour >= 24) {
        hour = 0;
        alarm = 0; //�Ϸ簡 ������ �ٽ� �˶��� �︱ �� �ְ� ����
        day++; //�Ϸ簡 ������ ���� ����
    }
}

#pragma interrupt_handler timer2_ovf_isr:iv_TIM2_OVF
void timer2_ovf_isr(void) {
    TCNT2 = 6; //���ֺ� 64�� ���� -> �ֱ� = 0.000004��
    cnt2++;
    if (cnt2 >= 10) {
        point2++; //�Ҽ��� ��°�ڸ� ����
        cnt2 = 0;
    }
    if (point2 == 10) {
        point1++; //�Ҽ��� ù°�ڸ� ����
        point2 = 0;
    }
    if (point1 == 10) {
        sw_sec++; //�����ġ �� ����
        point1 = 0;
    }
    if (sw_sec == 60) {
        sw_min++; //�����ġ �� ����
        sw_sec = 0;
    }
    if (sw_min == 60) {
        sw_hour++; //�����ġ �� ����
        sw_min = 0;
    }
    if (sw_hour == 24) {
        sw_hour = 0; //�����ġ�� 24�ø� �Ѿ�� 0�÷� �ٽ� ǥ��
    }
}
#pragma interrupt_handler int0_isr:iv_INT0
void int0_isr(void) {
    zulu++; //���� ��� ���� ǥ�ؽð� ����(¦���� ���), �ѹ� �� ���� ��� �ѱ� �ð��� ����(Ȧ���� ���)
}
#pragma interrupt_handler int1_isr:iv_INT1
void int1_isr(void) {
    sw_cnt++; //�ѹ� ���� ��� �����ġ ��� (sw_cnt = 1)
    if (sw_cnt == 2) { //�ѹ� �� ���� ���, Ÿ�̸�2�� �����÷ο� ��Ʈ enable(�����ġ �۵�)
        TIMSK = 0x41;
    }
    else if (sw_cnt == 3) { //�ѹ� �� ���� ���, Ÿ�̸�2�� �����÷ο� ��Ʈ disable(�����ġ ����)
        TIMSK = 0x01;
    }
    else if (sw_cnt == 4) { //�ѹ� �� ������� �����ġ ��� ����
        sw_cnt = 0;
    }
}
#pragma interrupt_handler int2_isr:iv_INT2
void int2_isr(void) {
    if (sw_cnt != 0) { //�����ġ ����� ��쿡�� ���ͷ�Ʈ �߻��ϰ� ����
        sw_cnt = 1; //�����ġ �ʱ�ȭ�� �� ���, sw_cnt�� 1�� ��������(�����ġ ���)
        /*�����ġ �ʱ�ȭ*/
        sw_hour = 0;
        sw_min = 0;
        sw_sec = 0;
        point1 = 0;
        point2 = 0;
    }
}
#pragma interrupt_handler int3_isr:iv_INT3
void int3_isr(void) {
    hour++; //���� ��� �ø� 1 ����
}
#pragma interrupt_handler int4_isr:iv_INT4
void int4_isr(void) {
    min++; //���� ��� ���� 1 ����
}
#pragma interrupt_handler int5_isr:iv_INT5
void int5_isr(void) {
    sec = 0; //���� ��� �ʸ� 0���� �ʱ�ȭ
}
#pragma interrupt_handler int6_isr:iv_INT6
void int6_isr(void) {
    alarm = -1; //���� ��� alarm = -1�� �����Ͽ� �Ϸ簡 ������ �������� �˶��� �� �︮�� �Ѵ�
    PORTA = 0x00; //��ƮA 0���ɿ� LOW ��� -> �˶��� ������.
}
