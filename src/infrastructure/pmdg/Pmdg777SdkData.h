#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777SDKDATA_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777SDKDATA_H

#include <cstddef>

#define PMDG_777X_DATA_NAME "PMDG_777X_Data"
#define PMDG_777X_DATA_ID 0x504D4447
#define PMDG_777X_DATA_DEFINITION 0x504D4448

#pragma pack(push, 4)

struct PMDG_777X_Data
{
    ////////////////////////////////////////////
    // Controls and indicators
    ////////////////////////////////////////////

    // Overhead Maintenance Panel
    //------------------------------------------

    // Backup Window Heat
    bool ICE_WindowHeatBackUp_Sw_OFF[2];

    // Standby Power
    unsigned char ELEC_StandbyPowerSw; // 0: OFF  1: AUTO  2: BAT

    // Flight Controls
    bool FCTL_WingHydValve_Sw_SHUT_OFF[3]; // left/right/center
    bool FCTL_TailHydValve_Sw_SHUT_OFF[3]; // left/right/center
    bool FCTL_annunTailHydVALVE_CLOSED[3]; // left/right/center
    bool FCTL_annunWingHydVALVE_CLOSED[3]; // left/right/center
    bool FCTL_PrimFltComputersSw_AUTO; // true: AUTO  false: DISC
    bool FCTL_annunPrimFltComputersDISC;

    // APU MAINT
    bool APU_Power_Sw_TEST;

    // EEC MAINT
    bool ENG_EECPower_Sw_TEST[2];

    // ELECTRICAL
    bool ELEC_TowingPower_Sw_BATT;
    bool ELEC_annunTowingPowerON_BATT;

    // CARGO TEMP SELECTORS
    unsigned char AIR_CargoTemp_Selector[2]; // aft / bulk  0=OFF  1=LOW  2=HIGH   AFT/BULK
    unsigned char AIR_CargoTemp_MainDeckFwd_Sel; // 0: C ... 60: W
    unsigned char AIR_CargoTemp_MainDeckAft_Sel; // 0: C ... 60: W
    unsigned char AIR_CargoTemp_LowerFwd_Sel; // 0: C ... 60: W
    unsigned char AIR_CargoTemp_LowerAft_Sel; // 0: C ... 60: W  67: HEAT H  70: HEAT OFF  73: HEAT L


    // Overhead Panel
    //------------------------------------------

    // ADIRU
    bool ADIRU_Sw_On;
    bool ADIRU_annunOFF;
    bool ADIRU_annunON_BAT;

    // Flight Controls
    bool FCTL_ThrustAsymComp_Sw_AUTO;
    bool FCTL_annunThrustAsymCompOFF;

    // Electrical
    bool ELEC_CabUtilSw;
    bool ELEC_annunCabUtilOFF;
    bool ELEC_IFEPassSeatsSw;
    bool ELEC_annunIFEPassSeatsOFF;
    bool ELEC_Battery_Sw_ON;
    bool ELEC_annunBattery_OFF;
    bool ELEC_annunAPU_GEN_OFF;
    bool ELEC_APUGen_Sw_ON;
    unsigned char ELEC_APU_Selector; // 0: OFF  1: ON  2: START
    bool ELEC_annunAPU_FAULT;
    bool ELEC_BusTie_Sw_AUTO[2];
    bool ELEC_annunBusTieISLN[2];
    bool ELEC_ExtPwrSw[2]; // primary/secondary - MOMENTARY SWITCHES
    bool ELEC_annunExtPowr_ON[2];
    bool ELEC_annunExtPowr_AVAIL[2];
    bool ELEC_Gen_Sw_ON[2];
    bool ELEC_annunGenOFF[2];
    bool ELEC_BackupGen_Sw_ON[2];
    bool ELEC_annunBackupGenOFF[2];
    bool ELEC_IDGDiscSw[2]; // MOMENTARY SWITCHES
    bool ELEC_annunIDGDiscDRIVE[2];

    // Wiper Selectors
    unsigned char WIPERS_Selector[2]; // left/right   0: OFF  1: INT  2: LOW  3:HIGH

    // Emergency Lights
    unsigned char LTS_EmerLightsSelector; // 0: OFF  1: ARMED  2: ON

    // Service Interphone
    bool COMM_ServiceInterphoneSw;

    // Passenger Oxygen
    bool OXY_PassOxygen_Sw_On;
    bool OXY_annunPassOxygenON;

    // Window Heat
    bool ICE_WindowHeat_Sw_ON[4]; // L-Side/L-Fwd/R-Fwd/R-Side
    bool ICE_annunWindowHeatINOP[4]; // L-Side/L-Fwd/R-Fwd/R-Side

    // Hydraulics
    bool HYD_RamAirTurbineSw;
    bool HYD_annunRamAirTurbinePRESS;
    bool HYD_annunRamAirTurbineUNLKD;
    bool HYD_PrimaryEngPump_Sw_ON[2];
    bool HYD_PrimaryElecPump_Sw_ON[2];
    unsigned char HYD_DemandElecPump_Selector[2]; // 0: OFF  1: AUTO  2: ON
    unsigned char HYD_DemandAirPump_Selector[2]; // 0: OFF  1: AUTO  2: ON
    bool HYD_annunPrimaryEngPumpFAULT[2];
    bool HYD_annunPrimaryElecPumpFAULT[2];
    bool HYD_annunDemandElecPumpFAULT[2];
    bool HYD_annunDemandAirPumpFAULT[2];

    // Passenger Signs
    unsigned char SIGNS_NoSmokingSelector; // 0: OFF  1: AUTO   2: ON
    unsigned char SIGNS_SeatBeltsSelector; // 0: OFF  1: AUTO   2: ON

    // Flight Deck Lights
    unsigned char LTS_DomeLightKnob; // Position 0...100
    unsigned char LTS_CircuitBreakerKnob; // Position 0...100
    unsigned char LTS_OvereadPanelKnob; // Position 0...100
    unsigned char LTS_GlareshieldPNLlKnob; // Position 0...100
    unsigned char LTS_GlareshieldFLOODKnob; // Position 0...100
    bool LTS_Storm_Sw_ON;
    bool LTS_MasterBright_Sw_ON;
    unsigned char LTS_MasterBrigntKnob; // Position 0...100
    unsigned char LTS_IndLightsTestSw; // 0: TEST  1: BRT  2: DIM

    // Exterior Lights
    bool LTS_LandingLights_Sw_ON[3]; // Left/Right/Nose
    bool LTS_Beacon_Sw_ON;
    bool LTS_NAV_Sw_ON;
    bool LTS_Logo_Sw_ON;
    bool LTS_Wing_Sw_ON;
    bool LTS_RunwayTurnoff_Sw_ON[2];
    bool LTS_Taxi_Sw_ON;
    bool LTS_Strobe_Sw_ON;

    // Engine, APU and Cargo Fire
    bool FIRE_CargoFire_Sw_Arm[2]; // FWD/AFT
    bool FIRE_annunCargoFire[2]; // FWD/AFT
    bool FIRE_CargoFireDisch_Sw; // MOMENTARY SWITCH
    bool FIRE_annunCargoDISCH;
    bool FIRE_FireOvhtTest_Sw; // MOMENTARY SWITCH
    unsigned char FIRE_APUHandle;
    // 0: IN (NORMAL)  1: PULLED OUT  2: TURNED LEFT  3: TURNED RIGHT  (2 & 3 ane momnentary positions)
    bool FIRE_APUHandleUnlock_Sw; // MOMENTARY SWITCH resets when handle pulled
    bool FIRE_annunAPU_BTL_DISCH;
    bool FIRE_EngineHandleIlluminated[2];
    bool FIRE_APUHandleIlluminated;
    bool FIRE_EngineHandleIsUnlocked[2];
    bool FIRE_APUHandleIsUnlocked;
    bool FIRE_annunMainDeckCargoFire;
    bool FIRE_annunCargoDEPR; // DEPR light in DEPR/DISCH guarded switch

    // Engine
    bool ENG_EECMode_Sw_NORM[2]; // left / right
    unsigned char ENG_Start_Selector[2]; // left / right  0: START  1: NORM
    bool ENG_Autostart_Sw_ON;
    bool ENG_annunALTN[2];
    bool ENG_annunAutostartOFF;

    // Fuel
    bool FUEL_CrossFeedFwd_Sw;
    bool FUEL_CrossFeedAft_Sw;
    bool FUEL_PumpFwd_Sw[2]; // left fwd / right fwd
    bool FUEL_PumpAft_Sw[2]; // left aft / right aft
    bool FUEL_PumpCtr_Sw[2]; // ctr left / ctr right
    bool FUEL_JettisonNozle_Sw[2]; // left / right
    bool FUEL_JettisonArm_Sw;
    bool FUEL_FuelToRemain_Sw_Pulled;
    unsigned char FUEL_FuelToRemain_Selector; // 0: DECR  1: Neutral  2: INCR
    bool FUEL_annunFwdXFEED_VALVE;
    bool FUEL_annunAftXFEED_VALVE;
    bool FUEL_annunLOWPRESS_Fwd[2]; // left fwd / right fwd
    bool FUEL_annunLOWPRESS_Aft[2]; // left aft / right aft
    bool FUEL_annunLOWPRESS_Ctr[2]; // ctr left / ctr right
    bool FUEL_annunJettisonNozleVALVE[2]; // left / right
    bool FUEL_annunArmFAULT;

    // Anti-Ice
    unsigned char ICE_WingAntiIceSw; // 0: OFF  1: AUTO  2: ON
    unsigned char ICE_EngAntiIceSw[2]; // 0: OFF  1: AUTO  2: ON


    // Air Conditioning
    bool AIR_Pack_Sw_AUTO[2]; // left / right
    bool AIR_TrimAir_Sw_On[2]; // left / right
    bool AIR_RecircFan_Sw_On[2]; // upper / lower
    unsigned char AIR_TempSelector[2]; // flt deck / cabin  0: C ... 60: W ... 70: MAN (flt deck only)
    bool AIR_AirCondReset_Sw_Pushed; // MOMENTARY action
    bool AIR_EquipCooling_Sw_AUTO;
    bool AIR_Gasper_Sw_On;
    bool AIR_annunPackOFF[2];
    bool AIR_annunTrimAirFAULT[2];
    bool AIR_annunEquipCoolingOVRD;
    bool AIR_AltnVentSw_ON;
    bool AIR_annunAltnVentFAULT;
    bool AIR_MainDeckFlowSw_NORM; // M/D FLOW  true: NORM  false: HIGH

    // Bleed Air
    bool AIR_EngBleedAir_Sw_AUTO[2]; // left engine / right engine
    bool AIR_APUBleedAir_Sw_AUTO;
    bool AIR_IsolationValve_Sw[2]; // left / right
    bool AIR_CtrIsolationValve_Sw; // left / right
    bool AIR_annunEngBleedAirOFF[2]; // left engine / right engine
    bool AIR_annunAPUBleedAirOFF;
    bool AIR_annunIsolationValveCLOSED[2]; // left / right
    bool AIR_annunCtrIsolationValveCLOSED;


    // Pressurization
    bool AIR_OutflowValve_Sw_AUTO[2]; // fwd / aft
    unsigned char AIR_OutflowValveManual_Selector[2]; // fwd / aft   0: OPEN  1: Neutral  2: CLOSE
    bool AIR_LdgAlt_Sw_Pulled;
    unsigned char AIR_LdgAlt_Selector; // 0: DECR  1: Neutral  2: INCR
    bool AIR_annunOutflowValve_MAN[2]; // fwd / aft


    // Forward panel
    //------------------------------------------

    // Center
    unsigned char GEAR_Lever; // 0: UP  1: DOWN
    bool GEAR_LockOvrd_Sw; // MOMENTARY SWITCH (resets when gear lever moved)
    bool GEAR_AltnGear_Sw_DOWN;
    bool GPWS_FlapInhibitSw_OVRD;
    bool GPWS_GearInhibitSw_OVRD;
    bool GPWS_TerrInhibitSw_OVRD;
    bool GPWS_RunwayOvrdSw_OVRD;
    bool GPWS_GSInhibit_Sw;
    bool GPWS_annunGND_PROX_top;
    bool GPWS_annunGND_PROX_bottom;
    unsigned char BRAKES_AutobrakeSelector; // 0: RTO  1: OFF  2: DISARM   3: "1" ... 5: MAX AUTO

    // Standby - ISFD  (all are MOMENTARY action switches)
    bool ISFD_Baro_Sw_Pushed;
    bool ISFD_RST_Sw_Pushed;
    bool ISFD_Minus_Sw_Pushed;
    bool ISFD_Plus_Sw_Pushed;
    bool ISFD_APP_Sw_Pushed;
    bool ISFD_HP_IN_Sw_Pushed;

    // Left
    bool ISP_Nav_L_Sw_CDU;
    bool ISP_DsplCtrl_L_Sw_Altn;
    bool ISP_AirDataAtt_L_Sw_Altn;
    unsigned char DSP_InbdDspl_L_Selector; //0: ND  1: NAV  2: MFD  3: EICAS
    bool EFIS_HdgRef_Sw_Norm;
    bool EFIS_annunHdgRefTRUE;
    int BRAKES_BrakePressNeedle; // Value 0...100 (corresponds to 0...4000 PSI)
    bool BRAKES_annunBRAKE_SOURCE;

    // Right
    bool ISP_Nav_R_Sw_CDU;
    bool ISP_DsplCtrl_R_Sw_Altn;
    bool ISP_AirDataAtt_R_Sw_Altn;
    unsigned char ISP_FMC_Selector; //0: LEFT   1: AUTO  2: RIGHT
    unsigned char DSP_InbdDspl_R_Selector; //0: EICAS  1: MFD   2: ND  3: PFD

    // Left & Right Sidewalls
    unsigned char AIR_ShoulderHeaterKnob[2]; // Left / Right  Position 0...100
    unsigned char AIR_FootHeaterSelector[2]; // Left / Right  0: OFF  1: LOW  2: HIGH
    unsigned char LTS_LeftFwdPanelPNLKnob; // Position 0...100
    unsigned char LTS_LeftFwdPanelFLOODKnob; // Position 0...100
    unsigned char LTS_LeftOutbdDsplBRIGHTNESSKnob; // Position 0...100
    unsigned char LTS_LeftInbdDsplBRIGHTNESSKnob; // Position 0...100

    unsigned char LTS_RightFwdPanelPNLKnob; // Position 0...100
    unsigned char LTS_RightFwdPanelFLOODKnob; // Position 0...100
    unsigned char LTS_RightInbdDsplBRIGHTNESSKnob; // Position 0...100
    unsigned char LTS_RightOutbdDsplBRIGHTNESSKnob; // Position 0...100


    // Chronometers (Left / Right)
    bool CHR_Chr_Sw_Pushed[2]; // MOMENTARY SWITCH
    bool CHR_TimeDate_Sw_Pushed[2]; // MOMENTARY SWITCH
    unsigned char CHR_TimeDate_Selector[2]; // 0: UTC  1: MAN
    unsigned char CHR_Set_Selector[2]; // 0: RUN  1: HLDY  2: MM  3: HD
    unsigned char CHR_ET_Selector[2]; // 0: RESET (MOMENTARY spring-loaded to HLD)  1: HLD  2: RUN


    // Glareshield
    //------------------------------------------

    // EFIS switches (left / right)
    bool EFIS_MinsSelBARO[2];
    bool EFIS_BaroSelHPA[2];
    unsigned char EFIS_VORADFSel1[2]; // 0: VOR  1: OFF  2: ADF
    unsigned char EFIS_VORADFSel2[2]; // 0: VOR  1: OFF  2: ADF
    unsigned char EFIS_ModeSel[2]; // 0: APP  1: VOR  2: MAP  3: PLAN
    unsigned char EFIS_RangeSel[2]; // 0: 10 ... 6: 640
    unsigned char EFIS_MinsKnob[2]; // 0..99
    unsigned char EFIS_BaroKnob[2]; // 0..99

    // EFIS MOMENTARY action (left / right)
    bool EFIS_MinsRST_Sw_Pushed[2];
    bool EFIS_BaroSTD_Sw_Pushed[2];
    bool EFIS_ModeCTR_Sw_Pushed[2];
    bool EFIS_RangeTFC_Sw_Pushed[2];
    bool EFIS_WXR_Sw_Pushed[2];
    bool EFIS_STA_Sw_Pushed[2];
    bool EFIS_WPT_Sw_Pushed[2];
    bool EFIS_ARPT_Sw_Pushed[2];
    bool EFIS_DATA_Sw_Pushed[2];
    bool EFIS_POS_Sw_Pushed[2];
    bool EFIS_TERR_Sw_Pushed[2];

    // MCP - Variables
    float MCP_IASMach; // Mach if < 10.0
    bool MCP_IASBlank;
    unsigned short MCP_Heading;
    unsigned short MCP_Altitude;
    short MCP_VertSpeed;
    float MCP_FPA;
    bool MCP_VertSpeedBlank;

    // MCP - Switches
    bool MCP_FD_Sw_On[2]; // left / right
    bool MCP_ATArm_Sw_On[2]; // left / right
    unsigned char MCP_BankLimitSel; // 0: AUTO  1: 5  2: 10 ... 5: 25
    bool MCP_AltIncrSel; // false: AUTO  true: 1000
    bool MCP_DisengageBar;
    unsigned char MCP_Speed_Dial; // 0 ... 99
    unsigned char MCP_Heading_Dial; // 0 ... 99
    unsigned char MCP_Altitude_Dial; // 0 ... 99
    unsigned char MCP_VS_Wheel; // 0 ... 99

    unsigned char MCP_HDGDial_Mode; // 0: Dial shows HDG, 1: Dial shows TRK
    unsigned char MCP_VSDial_Mode; // 0: Dial shows VS, 1: Dial shows FPA

    // MCP - MOMENTARY action switches
    bool MCP_AP_Sw_Pushed[2]; // left / right
    bool MCP_CLB_CON_Sw_Pushed;
    bool MCP_AT_Sw_Pushed;
    bool MCP_LNAV_Sw_Pushed;
    bool MCP_VNAV_Sw_Pushed;
    bool MCP_FLCH_Sw_Pushed;
    bool MCP_HDG_HOLD_Sw_Pushed;
    bool MCP_VS_FPA_Sw_Pushed;
    bool MCP_ALT_HOLD_Sw_Pushed;
    bool MCP_LOC_Sw_Pushed;
    bool MCP_APP_Sw_Pushed;
    bool MCP_Speeed_Sw_Pushed;
    bool MCP_Heading_Sw_Pushed;
    bool MCP_Altitude_Sw_Pushed;
    bool MCP_IAS_MACH_Toggle_Sw_Pushed;
    bool MCP_HDG_TRK_Toggle_Sw_Pushed;
    bool MCP_VS_FPA_Toggle_Sw_Pushed;

    // MCP - Annunciator lights
    bool MCP_annunAP[2]; // left / right
    bool MCP_annunAT;
    bool MCP_annunLNAV;
    bool MCP_annunVNAV;
    bool MCP_annunFLCH;
    bool MCP_annunHDG_HOLD;
    bool MCP_annunVS_FPA;
    bool MCP_annunALT_HOLD;
    bool MCP_annunLOC;
    bool MCP_annunAPP;

    // Display Select Panel	- These are all MOMENTARY SWITCHES
    bool DSP_L_INBD_Sw;
    bool DSP_R_INBD_Sw;
    bool DSP_LWR_CTR_Sw;
    bool DSP_ENG_Sw;
    bool DSP_STAT_Sw;
    bool DSP_ELEC_Sw;
    bool DSP_HYD_Sw;
    bool DSP_FUEL_Sw;
    bool DSP_AIR_Sw;
    bool DSP_DOOR_Sw;
    bool DSP_GEAR_Sw;
    bool DSP_FCTL_Sw;
    bool DSP_CAM_Sw;
    bool DSP_CHKL_Sw;
    bool DSP_COMM_Sw;
    bool DSP_NAV_Sw;
    bool DSP_CANC_RCL_Sw;
    bool DSP_annunL_INBD;
    bool DSP_annunR_INBD;
    bool DSP_annunLWR_CTR;

    // Master Warning/Caution
    bool WARN_Reset_Sw_Pushed[2]; // MOMENTARY action
    bool WARN_annunMASTER_WARNING[2];
    bool WARN_annunMASTER_CAUTION[2];


    // Forward Aisle Stand Panel
    //------------------------------------------

    bool ISP_DsplCtrl_C_Sw_Altn;
    unsigned char LTS_UpperDsplBRIGHTNESSKnob; // Position 0...100
    unsigned char LTS_LowerDsplBRIGHTNESSKnob; // Position 0...100
    bool EICAS_EventRcd_Sw_Pushed; // MOMENTARY action

    // CDU (Left/Right/Center)
    bool CDU_annunEXEC[3];
    bool CDU_annunDSPY[3];
    bool CDU_annunFAIL[3];
    bool CDU_annunMSG[3];
    bool CDU_annunOFST[3];
    unsigned char CDU_BrtKnob[3]; // 0: DecreasePosition 1: Neutral  2: Increase


    // Control Stand
    //------------------------------------------

    bool FCTL_AltnFlaps_Sw_ARM;
    unsigned char FCTL_AltnFlaps_Control_Sw; // 0: RET  1: OFF  2: EXT
    bool FCTL_StabCutOutSw_C_NORMAL;
    bool FCTL_StabCutOutSw_R_NORMAL;
    unsigned char FCTL_AltnPitch_Lever; // 0: NOSE DOWN  1: NEUTRAL  2: NOSE UP
    unsigned char FCTL_Speedbrake_Lever; // Position 0...100  0: DOWN,  25: ARMED, 26...100: DEPLOYED
    unsigned char FCTL_Flaps_Lever; // 0: UP  1: 1  2: 5  3: 15  4: 20  5: 25  6: 30
    bool ENG_FuelControl_Sw_RUN[2];
    bool BRAKES_ParkingBrakeLeverOn;


    // Aft Aisle Stand Panel
    //------------------------------------------

    // Audio Control Panels								// Comm Systems: 0=VHFL 1=VHFC 2=VHFR 3=FLT 4=CAB 5=PA 6=HFL 7=HFR 8=SAT1 9=SAT2 10=SPKR 11=VOR/ADF 12=APP
    unsigned char COMM_SelectedMic[3];
    // array: 0=capt, 1=F/O, 2=observer  values: 0..9 (VHF..SAT2) as listed above; -1 if no MIC is selected
    unsigned short COMM_ReceiverSwitches[3];
    // array: 0=capt, 1=F/O, 2=observer.  Bit mask for selected receivers with bits indicating: 0=VHFL 1=VHFC 2=VHFR 3=FLT 4=CAB 5=PA 6=HFL 7=HFR 8=SAT1 9=SAT2 10=SPKR 11=VOR/ADF 12=APP
    unsigned char COMM_OBSAudio_Selector; // 0: CAPT  1: NORMAL  2: F/O

    // Radio Control Panels								// arrays: 0=capt, 1=F/O, 2=observer
    unsigned char COMM_SelectedRadio[3]; // 0: VHFL  1: VHFC  2: VHFL  3: HFL  5: HFR (4 not used)
    bool COMM_RadioTransfer_Sw_Pushed[3]; // MOMENTARY action
    bool COMM_RadioPanelOff[3];
    bool COMM_annunAM[3];

    // TCAS Panel
    bool XPDR_XpndrSelector_R; // true: R     false: L
    bool XPDR_AltSourceSel_ALTN; // true: ALTN  false: NORM
    unsigned char XPDR_ModeSel; // 0: STBY  1: ALT RPTG OFF  2: XPNDR  3: TA ONLY  4: TA/RA
    bool XPDR_Ident_Sw_Pushed; // MOMENTARY action

    // Engine Fire
    unsigned char FIRE_EngineHandle[2];
    // ENG 1/ENG2   0: IN (NORMAL)  1: PULLED OUT  2: TURNED LEFT  3: TURNED RIGHT  (2 & 3 are momenentary positions)
    bool FIRE_EngineHandleUnlock_Sw[2]; // ENG 1/ENG2   MOMENTARY SWITCH resets when handle pulled
    bool FIRE_annunENG_BTL_DISCH[2]; // ENG 1/ENG2

    // Aileron & Rudder Trim
    unsigned char FCTL_AileronTrim_Switches;
    // 0: LEFT WING DOWN  1: NEUTRAL  2: RIGHT WING DOWN (both switches move together)
    unsigned char FCTL_RudderTrim_Knob; // 0: NOSE LEFT  1: NEUTRAL  2: NOSE RIGHT
    bool FCTL_RudderTrimCancel_Sw_Pushed; // MOMENTARY action

    // Evacuation Panel
    bool EVAC_Command_Sw_ON;
    bool EVAC_PressToTest_Sw_Pressed;
    bool EVAC_HornSutOff_Sw_Pulled;
    bool EVAC_LightIlluminated;


    // Aisle Stand PNL/FLOOD & Floor lights
    unsigned char LTS_AisleStandPNLKnob; // Position 0...100
    unsigned char LTS_AisleStandFLOODKnob; // Position 0...100
    unsigned char LTS_FloorLightsSw; // 0: BRT  1: OFF  2: DIM


    // Door state
    // Possible values are, 0: open, 1: closed, 2: closed and armed, 3: closing, 4: opening.
    // The array contains these doors:
    //  0: Entry 1L,
    //  1: Entry 1R,
    //  2: Entry 2L,
    //  3: Entry 2R,
    //  4: Entry 3L,				(This is the door aft of the wing. It is marked 4L on -300)
    //  5: Entry 3R,
    //  6: Entry 4L,				(marked 5L on -300)
    //  7: Entry 4R,
    //  8: Entry 5L,
    //  9: Entry 5R,
    // 10: Cargo Fwd,
    // 11: Cargo Aft,
    // 12: Cargo Main,				(Freighter)
    // 13: Cargo Bulk,
    // 14: Avionics Access,
    // 15: EE Access
    unsigned char DOOR_state[16];
    bool DOOR_CockpitDoorOpen;

    // Additional variables
    //------------------------------------------

    bool ENG_StartValve[2]; // true: valve open
    float AIR_DuctPress[2]; // PSI
    float FUEL_QtyCenter; // LBS
    float FUEL_QtyLeft; // LBS
    float FUEL_QtyRight; // LBS
    float FUEL_QtyAux; // LBS
    bool IRS_aligned; // at least one IRU is aligned

    bool EFIS_BaroMinimumsSet[2];
    // left/right control panel. Note: check EFIS_MinsSelBARO[2] to determine if the active minimums is BARO or RADIO
    int EFIS_BaroMinimums[2];
    bool EFIS_RadioMinimumsSet[2];
    int EFIS_RadioMinimums[2];

    // Display formats selected on the display units
    // Values are:
    // 	0:	Off,
    // 	1:	Blank,
    // 	2:	PFD,
    // 	3:	ND,
    // 	4:	EICAS,
    // 	5:	ENG,
    // 	6:	STAT,
    // 	7:	CHKL,
    // 	8:	COMM,
    // 	9:	CAM,
    // 10:	ELEC,
    // 11:	HYD,
    // 12:	FUEL,
    // 13:	AIR,
    // 14:	DOOR,
    // 15:	GEAR,
    // 16:	FCTL
    unsigned char EFIS_Display[6];
    // Display units:  0: Capt outboard, 1: Capt inboard, 2: Upper, 3: Lower, 4: FO Inboard, 5: FO Outboard

    unsigned char AircraftModel; // 1: -200  2: -200ER  3: -300  4: -200LR  5: 777F  6: -300ER
    bool WeightInKg; // false: LBS  true: KG
    bool GPWS_V1CallEnabled; // GPWS V1 call-out option enabled
    bool GroundConnAvailable; // can connect/disconnect ground air/electrics

    unsigned char FMC_TakeoffFlaps; // degrees, 0 if not set
    unsigned char FMC_V1; // knots, 0 if not set
    unsigned char FMC_VR; // knots, 0 if not set
    unsigned char FMC_V2; // knots, 0 if not set
    unsigned short FMC_ThrustRedAlt; // 1: FLAPS 1,  5: FLAPS 5,  otherwise altitude in ft
    unsigned short FMC_AccelerationAlt; // ft
    unsigned short FMC_EOAccelerationAlt; // ft
    unsigned char FMC_LandingFlaps; // degrees, 0 if not set
    unsigned char FMC_LandingVREF; // knots, 0 if not set
    unsigned short FMC_CruiseAlt; // ft, 0 if not set
    short FMC_LandingAltitude; // ft; -32767 if not available
    unsigned short FMC_TransitionAlt; // ft
    unsigned short FMC_TransitionLevel; // ft
    bool FMC_PerfInputComplete;
    float FMC_DistanceToTOD; // nm; 0.0 if passed, negative if n/a
    float FMC_DistanceToDest; // nm; negative if n/a
    char FMC_flightNumber[9];
    bool WheelChocksSet;
    bool APURunning;

    // FMC thrust limit mode
    // Values are:
    //  0:  TO,
    //  1:  TO 1,
    //  2:  TO 2,
    //  3:  TO B,
    //  4:  CLB,
    //  5:  CLB 1,
    //  6:  CLB 2,
    //  7:  CRZ,
    //  8:  CON,
    //  9:  G/A,
    // 10:  D-TO,
    // 11:  D-TO 1,
    // 12:  D-TO 2,
    // 13:  A-TO,
    // 14:  A-TO 1,
    // 15:  A-TO 2,
    // 16:  A-TO B
    unsigned char FMC_ThrustLimitMode;

    // Normal checklist completion status
    // Array elements are:
    // 	0:  PREFLIGHT,
    // 	1:  BEFORE_START,
    // 	2:  BEFORE_TAXI,
    // 	3:  BEFORE_TAKEOFF,
    // 	4:  AFTER_TAKEOFF,
    // 	5:  DESCENT,
    // 	6:  APPROACH,
    // 	7:  LANDING,
    // 	8:  SHUTDOWN,
    // 	9:  SECURE

    bool ECL_ChecklistComplete[10];

    unsigned char reserved[84];
};

#pragma pack(pop)

static_assert(sizeof(PMDG_777X_Data) == 684, "PMDG_777X_Data must marshal to 684 bytes (C# Pack=4 cross-check)");

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777SDKDATA_H
