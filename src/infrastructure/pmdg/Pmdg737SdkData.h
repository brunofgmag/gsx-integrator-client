#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737SDKDATA_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737SDKDATA_H

#include <cstddef>

#define PMDG_NG3_DATA_NAME "PMDG_NG3_Data"
#define PMDG_NG3_DATA_ID 0x4E473331
#define PMDG_NG3_DATA_DEFINITION 0x4E473332

#pragma pack(push, 4)

struct PMDG_NG3_Data
{
    ////////////////////////////////////////////
    // Controls and indicators
    ////////////////////////////////////////////

    // Aft overhead
    //------------------------------------------

    // ADIRU
    unsigned char    IRS_DisplaySelector;                // Positions 0..4
    bool            IRS_SysDisplay_R;                    // false: L  true: R
    bool            IRS_annunGPS;                        
    bool            IRS_annunALIGN[2];                        
    bool            IRS_annunON_DC[2];                        
    bool            IRS_annunFAULT[2];                        
    bool            IRS_annunDC_FAIL[2];                        
    unsigned char    IRS_ModeSelector[2];                // 0: OFF  1: ALIGN  2: NAV  3: ATT
    bool            IRS_aligned;                        // at least one IRU is aligned
    char            IRS_DisplayLeft[7];                    // Left display string, zero terminated
    char            IRS_DisplayRight[8];                // Right display string, zero terminated 
    bool            IRS_DisplayShowsDots;                // True if the degrees and decimal dot symbols are shown on the IRS display

    // AFS
    bool            AFS_AutothrottleServosConnected;    // True if the autothrottle system is driving the thrust levers
    bool            AFS_ControlsPitch;                    // The autoflight system is actively controlling pitch
    bool            AFS_ControlsRoll;


    // PSEU
    bool            WARN_annunPSEU;                        

    // Service Interphone
    bool            COMM_ServiceInterphoneSw;

    // Lights
    unsigned char    LTS_DomeWhiteSw;                    // 0: DIM  1: OFF  2: BRIGHT

    // Engine
    bool            ENG_EECSwitch[2];
    bool            ENG_annunREVERSER[2];
    bool            ENG_annunENGINE_CONTROL[2];
    bool            ENG_annunALTN[2];
    bool            ENG_StartValve[2];                    // true: valve open

    // Oxygen
    unsigned char    OXY_Needle;                            // Position 0...240
    bool            OXY_SwNormal;                        // true: NORMAL  false: ON
    bool            OXY_annunPASS_OXY_ON;

    // Gear
    bool            GEAR_annunOvhdLEFT;
    bool            GEAR_annunOvhdNOSE;
    bool            GEAR_annunOvhdRIGHT;

    // Flight recorder
    bool            FLTREC_SwNormal;                    // true: NORMAL  false: TEST
    bool            FLTREC_annunOFF;

    bool            CVR_annunTEST;                        // CVR TEST indicator


    // Forward overhead
    //------------------------------------------

    // Flight Controls
    unsigned char    FCTL_FltControl_Sw[2];                // 0: STBY/RUD  1: OFF  2: ON
    bool            FCTL_Spoiler_Sw[2];                    // true: ON  false: OFF  
    bool            FCTL_YawDamper_Sw;
    bool            FCTL_AltnFlaps_Sw_ARM;                // true: ARM  false: OFF
    unsigned char    FCTL_AltnFlaps_Control_Sw;            // 0: UP  1: OFF  2: DOWN
    bool            FCTL_annunFC_LOW_PRESSURE[2];        // FLT CONTROL
    bool            FCTL_annunYAW_DAMPER;    
    bool            FCTL_annunLOW_QUANTITY;    
    bool            FCTL_annunLOW_PRESSURE;    
    bool            FCTL_annunLOW_STBY_RUD_ON;    
    bool            FCTL_annunFEEL_DIFF_PRESS;    
    bool            FCTL_annunSPEED_TRIM_FAIL;    
    bool            FCTL_annunMACH_TRIM_FAIL;    
    bool            FCTL_annunAUTO_SLAT_FAIL;    

    // Navigation/Displays
    unsigned char    NAVDIS_VHFNavSelector;                // 0: BOTH ON 1  1: NORMAL  2: BOTH ON 2
    unsigned char    NAVDIS_IRSSelector;                    // 0: BOTH ON L  1: NORMAL  2: BOTH ON R
    unsigned char    NAVDIS_FMCSelector;                    // 0: BOTH ON L  1: NORMAL  2: BOTH ON R
    unsigned char    NAVDIS_SourceSelector;                // 0: ALL ON 1   1: AUTO    2: ALL ON 2
    unsigned char    NAVDIS_ControlPaneSelector;            // 0: BOTH ON 1  1: NORMAL  2: BOTH ON 2
    unsigned int    ADF_StandbyFrequency;                // Standby frequency multiplied by 10

    // Fuel
    float            FUEL_FuelTempNeedle;                // Value
    bool            FUEL_CrossFeedSw;
    bool            FUEL_PumpFwdSw[2];                    // left fwd / right fwd
    bool            FUEL_PumpAftSw[2];                    // left aft / right aft
    bool            FUEL_PumpCtrSw[2];                    // ctr left / ctr right
    bool            FUEL_AuxFwd[2];                        // aux fwd A and aux fwd B
    bool            FUEL_AuxAft[2];                        //aux aft A and aux aft B
    bool            FUEL_FWDBleed;
    bool            FUEL_AFTBleed;
    bool            FUEL_GNDXfr;
    unsigned char    FUEL_annunENG_VALVE_CLOSED[2];        // 0: Closed  1: Open  2: In transit (bright)
    unsigned char    FUEL_annunSPAR_VALVE_CLOSED[2];        // 0: Closed  1: Open  2: In transit (bright)
    bool            FUEL_annunFILTER_BYPASS[2];
    unsigned char    FUEL_annunXFEED_VALVE_OPEN;            // 0: Closed  1: Open  2: In transit (dim)
    bool            FUEL_annunLOWPRESS_Fwd[2];
    bool            FUEL_annunLOWPRESS_Aft[2];
    bool            FUEL_annunLOWPRESS_Ctr[2];
    float            FUEL_QtyCenter;                        // LBS
    float            FUEL_QtyLeft;                        // LBS
    float            FUEL_QtyRight;                        // LBS

    // Electrical
    bool            ELEC_annunBAT_DISCHARGE;
    bool            ELEC_annunTR_UNIT;
    bool            ELEC_annunELEC;
    unsigned char    ELEC_DCMeterSelector;                // 0: STBY PWR  1: BAT BUS ... 7: TEST
    unsigned char    ELEC_ACMeterSelector;                // 0: STBY PWR  1: GND PWR ... 6: TEST
    unsigned char    ELEC_BatSelector;                    // 0: OFF  1: BAT  2: ON
    bool            ELEC_CabUtilSw;
    bool            ELEC_IFEPassSeatSw;
    bool            ELEC_annunDRIVE[2];
    bool            ELEC_annunSTANDBY_POWER_OFF;
    bool            ELEC_IDGDisconnectSw[2];
    unsigned char    ELEC_StandbyPowerSelector;            // 0: BAT  1: OFF  2: AUTO
    bool            ELEC_annunGRD_POWER_AVAILABLE;
    bool            ELEC_GrdPwrSw;
    bool            ELEC_BusTransSw_AUTO;
    bool            ELEC_GenSw[2];
    bool            ELEC_APUGenSw[2];
    bool            ELEC_annunTRANSFER_BUS_OFF[2];
    bool            ELEC_annunSOURCE_OFF[2];
    bool            ELEC_annunGEN_BUS_OFF[2];
    bool            ELEC_annunAPU_GEN_OFF_BUS;
    char            ELEC_MeterDisplayTop[13];            // Top line of the display: 3 groups of 4 digits (or symbols) + terminating zero 
    char            ELEC_MeterDisplayBottom[13];        // Bottom line of the display
    bool            ELEC_BusPowered[16];                // True if the corresponding bus is powered:
                                                        // DC HOT BATT            0    
                                                        // DC HOT BATT SWITCHED    1    
                                                        // DC BATT BUS            2    
                                                        // DC STANDBY BUS        3    
                                                        // DC BUS 1                4    
                                                        // DC BUS 2                5    
                                                        // DC GROUND SVC        6
                                                        // AC TRANSFER 1        7
                                                        // AC TRANSFER 2        8
                                                        // AC GROUND SVC 1        9
                                                        // AC GROUND SVC 2        10
                                                        // AC MAIN 1            11
                                                        // AC MAIN 2            12
                                                        // AC GALLEY 1            13
                                                        // AC GALLEY 2            14
                                                        // AC STANDBY            15

    // APU
    float            APU_EGTNeedle;                        // Value
    bool            APU_annunMAINT;
    bool            APU_annunLOW_OIL_PRESSURE;
    bool            APU_annunFAULT;
    bool            APU_annunOVERSPEED;

    // Wipers
    unsigned char    OH_WiperLSelector;                    // 0: PARK  1: INT  2: LOW  3:HIGH
    unsigned char    OH_WiperRSelector;                    // 0: PARK  1: INT  2: LOW  3:HIGH

    // Center overhead controls & indicators
    unsigned char    LTS_CircuitBreakerKnob;                // Position 0...150
    unsigned char    LTS_OvereadPanelKnob;                // Position 0...150
    bool            AIR_EquipCoolingSupplyNORM;
    bool            AIR_EquipCoolingExhaustNORM;
    bool            AIR_annunEquipCoolingSupplyOFF;
    bool            AIR_annunEquipCoolingExhaustOFF;
    bool            LTS_annunEmerNOT_ARMED;
    unsigned char    LTS_EmerExitSelector;                // 0: OFF  1: ARMED  2: ON
    unsigned char    COMM_NoSmokingSelector;                // 0: OFF  1: AUTO   2: ON
    unsigned char    COMM_FastenBeltsSelector;            // 0: OFF  1: AUTO   2: ON
    bool            COMM_annunCALL;
    bool            COMM_annunPA_IN_USE;

    // Anti-ice
    bool            ICE_annunOVERHEAT[4];
    bool            ICE_annunON[4];
    bool            ICE_WindowHeatSw[4];
    bool            ICE_annunCAPT_PITOT;
    bool            ICE_annunL_ELEV_PITOT;
    bool            ICE_annunL_ALPHA_VANE;
    bool            ICE_annunL_TEMP_PROBE;
    bool            ICE_annunFO_PITOT;
    bool            ICE_annunR_ELEV_PITOT;
    bool            ICE_annunR_ALPHA_VANE;
    bool            ICE_annunAUX_PITOT;
    bool            ICE_ProbeHeatSw[2];
    bool            ICE_annunVALVE_OPEN[2];
    bool            ICE_annunCOWL_ANTI_ICE[2];
    bool            ICE_annunCOWL_VALVE_OPEN[2];
    bool            ICE_WingAntiIceSw;
    bool            ICE_EngAntiIceSw[2];
    int                ICE_WindowHeatTestSw;                // 0: OVHT  1: Neutral  2: PWR TEST

    // Hydraulics
    bool            HYD_annunLOW_PRESS_eng[2];
    bool            HYD_annunLOW_PRESS_elec[2];
    bool            HYD_annunOVERHEAT_elec[2];
    bool            HYD_PumpSw_eng[2];
    bool            HYD_PumpSw_elec[2];

    // Air systems
    unsigned char    AIR_TempSourceSelector;                // Positions 0..6
    bool            AIR_TrimAirSwitch;
    bool            AIR_annunZoneTemp[3];
    bool            AIR_annunDualBleed;        
    bool            AIR_annunRamDoorL;        
    bool            AIR_annunRamDoorR;        
    bool            AIR_RecircFanSwitch[2];            
    unsigned char   AIR_PackSwitch[2];                    // 0=OFF  1=AUTO  2=HIGH
    bool            AIR_BleedAirSwitch[2];            
    bool            AIR_APUBleedAirSwitch;            
    unsigned char    AIR_IsolationValveSwitch;            // 0=CLOSE  1=AUTO  2=OPEN
    bool            AIR_annunPackTripOff[2];        
    bool            AIR_annunWingBodyOverheat[2];        
    bool            AIR_annunBleedTripOff[2];    
    bool            AIR_annunAUTO_FAIL;
    bool            AIR_annunOFFSCHED_DESCENT;
    bool            AIR_annunALTN;
    bool            AIR_annunMANUAL;
    float            AIR_DuctPress[2];                    // PSI
    float            AIR_DuctPressNeedle[2];                // Value - PSI
    float            AIR_CabinAltNeedle;                    // Value - ft
    float            AIR_CabinDPNeedle;                    // Value - PSI
    float            AIR_CabinVSNeedle;                    // Value - ft/min
    float            AIR_CabinValveNeedle;                // Value - 0 (closed) .. 1 (open)
    float            AIR_TemperatureNeedle;                // Value - degrees C
    char            AIR_DisplayFltAlt[6];                // Pressurization system FLT ALT window, zero terminated, can be blank or show dashes or show test pattern
    char            AIR_DisplayLandAlt[6];                // Pressurization system LAND ALT window, zero terminated, can be blank or show dashes or show test pattern

    // Doors
    bool            DOOR_annunFWD_ENTRY;
    bool            DOOR_annunFWD_SERVICE;
    bool            DOOR_annunAIRSTAIR;
    bool            DOOR_annunLEFT_FWD_OVERWING;
    bool            DOOR_annunRIGHT_FWD_OVERWING;
    bool            DOOR_annunFWD_CARGO;
    bool            DOOR_annunEQUIP;
    bool            DOOR_annunLEFT_AFT_OVERWING;
    bool            DOOR_annunRIGHT_AFT_OVERWING;
    bool            DOOR_annunAFT_CARGO;
    bool            DOOR_annunAFT_ENTRY;
    bool            DOOR_annunAFT_SERVICE;

    unsigned int    AIR_FltAltWindow;                    // WARNING obsolete - use AIR_DisplayFltAlt instead
    unsigned int    AIR_LandAltWindow;                    // WARNING obsolete - use AIR_DisplayLandAlt instead
    unsigned int    AIR_OutflowValveSwitch;                // 0=CLOSE  1=NEUTRAL  2=OPEN
    unsigned int    AIR_PressurizationModeSelector;        // 0=AUTO  1=ALTN  2=MAN

    // Bottom overhead
    unsigned char    LTS_LandingLtRetractableSw[2];        // 0: RETRACT  1: EXTEND  2: ON
    bool            LTS_LandingLtFixedSw[2];
    bool            LTS_RunwayTurnoffSw[2];
    bool            LTS_TaxiSw;
    unsigned char    APU_Selector;                        // 0: OFF  1: ON  2: START
    unsigned char    ENG_StartSelector[2];                // 0: GRD  1: OFF  2: CONT  3: FLT
    unsigned char    ENG_IgnitionSelector;                // 0: IGN L  1: BOTH  2: IGN R
    bool            LTS_LogoSw;
    unsigned char    LTS_PositionSw;                        // 0: STEADY  1: OFF  2: STROBE&STEADY
    bool            LTS_AntiCollisionSw;
    bool            LTS_WingSw;
    bool            LTS_WheelWellSw;


    // Glareshield
    //------------------------------------------

    // Warnings
    bool            WARN_annunFIRE_WARN[2];
    bool            WARN_annunMASTER_CAUTION[2];
    bool            WARN_annunFLT_CONT;
    bool            WARN_annunIRS;
    bool            WARN_annunFUEL;
    bool            WARN_annunELEC;
    bool            WARN_annunAPU;
    bool            WARN_annunOVHT_DET;
    bool            WARN_annunANTI_ICE;
    bool            WARN_annunHYD;
    bool            WARN_annunDOORS;
    bool            WARN_annunENG;
    bool            WARN_annunOVERHEAD;
    bool            WARN_annunAIR_COND;

    // EFIS control panels
    bool            EFIS_MinsSelBARO[2];
    bool            EFIS_BaroSelHPA[2];
    unsigned char    EFIS_VORADFSel1[2];                    // 0: VOR  1: OFF  2: ADF
    unsigned char    EFIS_VORADFSel2[2];                    // 0: VOR  1: OFF  2: ADF
    unsigned char    EFIS_ModeSel[2];                    // 0: APP  1: VOR  2: MAP  3: PLAn
    unsigned char    EFIS_RangeSel[2];                    // 0: 5 ... 7: 640

    // Mode control panel
    unsigned short    MCP_Course[2];
    float            MCP_IASMach;                        // Mach if < 10.0
    bool            MCP_IASBlank;
    bool            MCP_IASOverspeedFlash;
    bool            MCP_IASUnderspeedFlash;
    unsigned short    MCP_Heading;
    unsigned short    MCP_Altitude;
    short            MCP_VertSpeed;
    bool            MCP_VertSpeedBlank;

    bool            MCP_FDSw[2];
    bool            MCP_ATArmSw;
    unsigned char    MCP_BankLimitSel;                    // 0: 10 ... 4: 30
    bool            MCP_DisengageBar;

    bool            MCP_annunFD[2];
    bool            MCP_annunATArm;
    bool            MCP_annunN1;
    bool            MCP_annunSPEED;
    bool            MCP_annunVNAV;
    bool            MCP_annunLVL_CHG;
    bool            MCP_annunHDG_SEL;
    bool            MCP_annunLNAV;
    bool            MCP_annunVOR_LOC;
    bool            MCP_annunAPP;
    bool            MCP_annunALT_HOLD;
    bool            MCP_annunVS;
    bool            MCP_annunCMD_A;
    bool            MCP_annunCWS_A;
    bool            MCP_annunCMD_B;
    bool            MCP_annunCWS_B;

    bool            MCP_indication_powered;                // true when the MCP is powered and the MCP windows are indicating

    // Forward panel
    //------------------------------------------
    bool            MAIN_NoseWheelSteeringSwNORM;        // false: ALT
    bool            MAIN_annunBELOW_GS[2];
    unsigned char    MAIN_MainPanelDUSel[2];                // 0: OUTBD PFD ... 4 MFD for Capt; reverse sequence for FO
    unsigned char    MAIN_LowerDUSel[2];                    // 0: ENG PRI ... 2 ND for Capt; reverse sequence for FO
    bool            MAIN_annunAP[2];                    // Red color. See MAIN_annunAP_Amber for amber color (added to the bottom of the struct)
    bool            MAIN_annunAP_Amber[2];                // Amber color
    bool            MAIN_annunAT[2];                    // Red color. See MAIN_annunAT_Amber for amber color (added to the bottom of the struct)
    bool            MAIN_annunAT_Amber[2];                // Amber color
    bool            MAIN_annunFMC[2];
    unsigned char    MAIN_DisengageTestSelector[2];            // 0: 1  1: OFF  2: 2
    bool            MAIN_annunSPEEDBRAKE_ARMED;
    bool            MAIN_annunSPEEDBRAKE_DO_NOT_ARM;
    bool            MAIN_annunSPEEDBRAKE_EXTENDED;
    bool            MAIN_annunSTAB_OUT_OF_TRIM;
    unsigned char    MAIN_LightsSelector;                // 0: TEST  1: BRT  2: DIM
    bool            MAIN_RMISelector1_VOR;
    bool            MAIN_RMISelector2_VOR;
    unsigned char    MAIN_N1SetSelector;                    // 0: 2  1: 1  2: AUTO  3: BOTH
    unsigned char    MAIN_SpdRefSelector;                // 0: SET  1: AUTO  2: V1  3: VR  4: WT  5: VREF  6: Bug  
    unsigned char    MAIN_FuelFlowSelector;                // 0: RESET  1: RATE  2: USED
    unsigned char    MAIN_AutobrakeSelector;                // 0: RTO  1: OFF ... 5: MAX
    bool            MAIN_annunANTI_SKID_INOP;
    bool            MAIN_annunAUTO_BRAKE_DISARM;
    bool            MAIN_annunLE_FLAPS_TRANSIT;
    bool            MAIN_annunLE_FLAPS_EXT;
    float            MAIN_TEFlapsNeedle[2];                // Value
    bool            MAIN_annunGEAR_transit[3];
    bool            MAIN_annunGEAR_locked[3];
    unsigned char    MAIN_GearLever;                        // 0: UP  1: OFF  2: DOWN
    float            MAIN_BrakePressNeedle;                // Value
    bool            MAIN_annunCABIN_ALTITUDE;
    bool            MAIN_annunTAKEOFF_CONFIG;

    bool            HGS_annun_AIII;
    bool            HGS_annun_NO_AIII;
    bool            HGS_annun_FLARE;    
    bool            HGS_annun_RO;
    bool            HGS_annun_RO_CTN;
    bool            HGS_annun_RO_ARM;
    bool            HGS_annun_TO;
    bool            HGS_annun_TO_CTN;
    bool            HGS_annun_APCH;
    bool            HGS_annun_TO_WARN;
    bool            HGS_annun_Bar;
    bool            HGS_annun_FAIL;

    // Lower forward panel
    //------------------------------------------
    unsigned char    LTS_MainPanelKnob[2];                // Position 0...150
    unsigned char    LTS_BackgroundKnob;                    // Position 0...150
    unsigned char    LTS_AFDSFloodKnob;                    // Position 0...150
    unsigned char    LTS_OutbdDUBrtKnob[2];                // Position 0...127
    unsigned char    LTS_InbdDUBrtKnob[2];                // Position 0...127
    unsigned char    LTS_InbdDUMapBrtKnob[2];            // Position 0...127
    unsigned char    LTS_UpperDUBrtKnob;                    // Position 0...127
    unsigned char    LTS_LowerDUBrtKnob;                    // Position 0...127
    unsigned char    LTS_LowerDUMapBrtKnob;                // Position 0...127

    bool            GPWS_annunINOP;
    bool            GPWS_FlapInhibitSw_NORM;
    bool            GPWS_GearInhibitSw_NORM;
    bool            GPWS_TerrInhibitSw_NORM;


    // Control Stand
    //------------------------------------------

    bool            CDU_annunEXEC[2];
    bool            CDU_annunCALL[2];
    bool            CDU_annunFAIL[2];
    bool            CDU_annunMSG[2];
    bool            CDU_annunOFST[2];
    unsigned char    CDU_BrtKnob[2];                        // Position 0...127

    unsigned char   COMM_Attend_PressCount;                // incremented with each button press
    unsigned char   COMM_GrdCall_PressCount;            // incremented with each button press
    unsigned char   COMM_SelectedMic[3];                // array: 0=capt, 1=F/O, 2=observer.
                                                        // values: 0=VHF1  1=VHF2  2=VHF3  3=HF1  4=HF2  5=FLT  6=SVC  7=PA
    unsigned int    COMM_ReceiverSwitches[3];            // Bit flags for selector receivers (see ACP_SEL_RECV_VHF1 etc): [0]=Capt, [1]=FO, [2]=Overhead
    bool            TRIM_StabTrimMainElecSw_NORMAL;
    bool            TRIM_StabTrimAutoPilotSw_NORMAL;
    bool            PED_annunParkingBrake;

    unsigned char    FIRE_OvhtDetSw[2];                    // 0: A  1: NORMAL  2: B
    bool            FIRE_annunENG_OVERHEAT[2];
    unsigned char    FIRE_DetTestSw;                        // 0: FAULT/INOP  1: neutral  2: OVHT/FIRE
    unsigned char    FIRE_HandlePos[3];                    // 0: In  1: Blocked  2: Out  3: Turned Left  4: Turned right
    bool            FIRE_HandleIlluminated[3];
    bool            FIRE_annunWHEEL_WELL;
    bool            FIRE_annunFAULT;
    bool            FIRE_annunAPU_DET_INOP;
    bool            FIRE_annunAPU_BOTTLE_DISCHARGE;
    bool            FIRE_annunBOTTLE_DISCHARGE[2];
    unsigned char    FIRE_ExtinguisherTestSw;            // 0: 1  1: neutral  2: 2
    bool            FIRE_annunExtinguisherTest[3];        // Left, Right, APU

    bool            CARGO_annunExtTest[2];                // Fwd, Aft
    unsigned char    CARGO_DetSelect[2];                    // 0: A  1: ORM  2: B
    bool            CARGO_ArmedSw[2];                
    bool            CARGO_annunFWD;                
    bool            CARGO_annunAFT;                
    bool            CARGO_annunDETECTOR_FAULT;                
    bool            CARGO_annunDISCH;            

    bool            HGS_annunRWY;
    bool            HGS_annunGS;
    bool            HGS_annunFAULT;
    bool            HGS_annunCLR;

    bool            XPDR_XpndrSelector_2;                // false: 1  true: 2
    bool            XPDR_AltSourceSel_2;                // false: 1  true: 2
    unsigned char    XPDR_ModeSel;                        // 0: STBY  1: ALT RPTG OFF ... 4: TA/RA
    bool            XPDR_annunFAIL;

    unsigned char    LTS_PedFloodKnob;                    // Position 0...150
    unsigned char    LTS_PedPanelKnob;                    // Position 0...150

    bool            TRIM_StabTrimSw_NORMAL;
    bool            PED_annunLOCK_FAIL;
    bool            PED_annunAUTO_UNLK;
    unsigned char    PED_FltDkDoorSel;                    // 0: UNLKD  1 AUTO pushed in  2: AUTO  3: DENY

    // FMS
    //------------------------------------------
    unsigned char    FMC_TakeoffFlaps;                    // degrees, 0 if not set
    unsigned char    FMC_V1;                                // knots, 0 if not set
    unsigned char    FMC_VR;                                // knots, 0 if not set
    unsigned char    FMC_V2;                                // knots, 0 if not set
    unsigned char    FMC_LandingFlaps;                    // degrees, 0 if not set
    unsigned char    FMC_LandingVREF;                    // knots, 0 if not set
    unsigned short  FMC_CruiseAlt;                        // ft, 0 if not set
    short            FMC_LandingAltitude;                // ft; -32767 if not available
    unsigned short  FMC_TransitionAlt;                    // ft
    unsigned short  FMC_TransitionLevel;                // ft
    bool            FMC_PerfInputComplete;
    float            FMC_DistanceToTOD;                    // nm; 0.0 if passed, negative if n/a
    float            FMC_DistanceToDest;                    // nm; negative if n/a
    char            FMC_flightNumber[9];


    // General and misc
    //------------------------------------------

    unsigned short  AircraftModel;                        // 1: -600  
                                                        // 2: -700  3: -700 BW  4: -700 SSW  
                                                        // 5: -800  6: -800 BW  7: -800 SSW  
                                                        // 8: -900  9: -900 BW  10: -900 SSW  11: -900ER BW  12: -900ER SSW 
                                                        // 13: -700 BDSF BW  14: -700 BDSF SSW  15: -800 BDSF BW  16: -800 BDSF SSW  17: -800 BCF BW  18 -800 BCF SSW  
                                                        // 19: -700 BBJ BW  20: -700 BBJ SSW  21: -800 BBJ BW  16: -800 BBJ SSW

    bool            WeightInKg;                            // false: LBS  true: KG
    bool            GPWS_V1CallEnabled;                    // GPWS V1 callout option enabled
    bool            GroundConnAvailable;                // can connect/disconnect ground air/electrics


    unsigned char   reserved[255];                        
};

#pragma pack(pop)

static_assert(sizeof(PMDG_NG3_Data) == 916, "PMDG_NG3_Data must marshal to 916 bytes (C# Pack=4 cross-check)");

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737SDKDATA_H
