/*--------------------------------------------------------*/
// GestPWM.c
/*--------------------------------------------------------*/
//	Description :	Gestion des PWM 
//			        pour TP1 2016-2017
//
//	Auteur 		: 	C. HUBER
//
//	Version		:	V1.1
//	Compilateur	:	XC32 V1.42 + Harmony 1.08
//
/*--------------------------------------------------------*/



#include "GestPWM.h"
#include "Mc32DriverLcd.h"
#include "Mc32DriverAdc.h"
#include "bsp.h"
#include "driver/oc/drv_oc_static.h"
#include "driver/tmr/drv_tmr_static.h"

S_pwmSettings PWMData;      // pour les settings

void GPWM_Initialize(S_pwmSettings *pData)
{
    // Init les data 
    
    // Init état du pont en H
    BSP_EnableHbrige();
    // Lancement des timers et OC
    // Start des OCs
    DRV_OC0_Start();
    DRV_OC1_Start();

    // Start des timers
    DRV_TMR0_Start();
    DRV_TMR1_Start();
    DRV_TMR2_Start();
    DRV_TMR3_Start();
   
}

// Obtention vitesse et angle (mise a jour des 4 champs de la structure)
void GPWM_GetSettings(S_pwmSettings *pData)	
{
    //Variable pour la lecture des 2 canaux de l'ADC
    S_ADCResults Resultats;
    
    //Tableaux pour stockage de 10 valeurs d'ADC
    uint16_t TabValADC1[10]; //AD1
    uint16_t TabValADC2[10]; //AD2
    
    //Variables de comptage
    static uint8_t i = 0; //variable statique pour ne pas la modifier après prochaine interruption
    uint8_t j = 0;
    
    //Variables pour moyennes
    uint16_t Moy1 = 0;
    uint16_t Moy2 = 0;
    
    // Variable pour la valeur brute
    uint16_t ValADC1 = 0;
    
    // Lecture du convertisseur AD (valeur canal 0)
    Resultats = BSP_ReadAllADC();
    
    //====================MOYENNES====================//
    //MOYENNES GLISSANTES
    for(i = 0; i < 10; i++)
    {
        TabValADC1[i] = Resultats.Chan0;
        TabValADC2[i] = Resultats.Chan1;
    }
    
    //Récup valeurs moyennes
    for(j = 0; j < 10; j++)
    {
        Moy1 += TabValADC1[j];
        Moy2 += TabValADC2[j];
    }
    
    Moy1 = Moy1 / 10;
    Moy2 = Moy2 / 10;
    
    //===============================================//
    // conversion  
    ValADC1 = (Moy1*198) / 1023;
    
    PWMData.SpeedSetting = ValADC1 - 99;
    if(PWMData.SpeedSetting < 0)
    {
        PWMData.absSpeed = PWMData.SpeedSetting * -1;
    }
    else
    {
        PWMData.absSpeed = PWMData.SpeedSetting;
    }
    
    // conversion  
    PWMData.absAngle = (Moy2*180) / 1023;
    
    PWMData.AngleSetting = PWMData.absAngle - 90;
}


// Affichage des information en exploitant la structure
void GPWM_DispSettings(S_pwmSettings *pData)
{
    // Affichage de la vitesse signée du moteur
    lcd_gotoxy(1,2);
    printf_lcd("SpeedSetting %3d", pData -> SpeedSetting);
    // Affichage de la vitesse absolute du moteur
    lcd_gotoxy(1,3);
    printf_lcd("absSpeed %7d", pData -> absSpeed);
    // Affichage de l'angle du servomoteur
    lcd_gotoxy(1,4);
    printf_lcd("Angle %10d", pData -> AngleSetting);
    
}

// Execution PWM et gestion moteur à partir des info dans structure
void GPWM_ExecPWM(S_pwmSettings *pData)
{
    // Variables locales
    uint32_t OC2_DutyCycle = 0;
    uint32_t OC3_DutyCycle = 0;
    
    // Gestion de la direction en focntion du signe de la vitesse
    // Bit8 de pData.SpeedSetting = au signe de la vitesse
    // Masquage du bit8 dans pData.SpeedSetting pour savoir la direction
    // Si bit8 = 1 -> direction CCW , sinon direction CW
    if(pData -> SpeedSetting > 0)
    {
        PLIB_PORTS_PinClear( PORTS_ID_0, PORT_CHANNEL_D, AIN1_HBRIDGE_BIT);
        PLIB_PORTS_PinSet( PORTS_ID_0, PORT_CHANNEL_D, AIN2_HBRIDGE_BIT);
        //AIN1_HBRIDGE_W = 0;   //AIN1 High
        //AIN2_HBRIDGE_W = 1;
    }
    else
    {
        PLIB_PORTS_PinSet( PORTS_ID_0, PORT_CHANNEL_D, AIN1_HBRIDGE_BIT);
        PLIB_PORTS_PinClear( PORTS_ID_0, PORT_CHANNEL_D, AIN2_HBRIDGE_BIT);
        //AIN1_HBRIDGE_W = 1;   //AIN1 High
        //AIN2_HBRIDGE_W = 0;
    }
    
    // Calcul pour la conversion de la valeur de pData.absSpeed en %
    OC2_DutyCycle = (( (float)2000 / (float)100) * (float)pData -> absSpeed)
            - 0.5;
    DRV_OC0_PulseWidthSet(OC2_DutyCycle);
    
    // Calcul pour la conversion de la valeur de pData.absSpeed en nombre
    // d'impulsions
    OC3_DutyCycle = (((float)9000 / (float)180) * (float)pData -> absAngle)
            + 2999.5;
    DRV_OC1_PulseWidthSet(OC3_DutyCycle);
    //DRV_OC1_CompareValuesDualSet(OC3_DutyCycle, 2999 );
}

// Execution PWM software
void GPWM_ExecPWMSoft(S_pwmSettings *pData)
{
    // Compteur pour la PWM
    static uint8_t PWM_Cnt = 0;
    
    PWM_Cnt = (PWM_Cnt + 1) % 100;
   
    if ( PWM_Cnt < pData->absSpeed)
    {
        BSP_LEDOn(BSP_LED_2);
    }
    else
    {
        BSP_LEDOff(BSP_LED_2);
    }
}


