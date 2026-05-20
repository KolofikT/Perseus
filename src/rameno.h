#pragma once

void fMoveManipulator(int iManipulator_ID, int iTarget_Tip_Absolute_X, int iTarget_Tip_Absolute_Y, int iBase_Angle);
void fMoveGrabber(int iManipulator_ID, int iPoloha_Grabberu);

extern int iOpen_Grabber_RA;
extern int iClose_Grabber_RA;
extern int iGrab_Battery;
extern int iDrop_Battery;
extern int iOpen_Grabber_TC; 
extern int iClose_Grabber_TC;