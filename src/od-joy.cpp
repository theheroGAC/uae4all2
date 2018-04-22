 /*
	* UAE - The Un*x Amiga Emulator
	*
	* Joystick emulation for Linux and BSD. They share too much code to
	* split this file.
	*
	* Copyright 1997 Bernd Schmidt
	* Copyright 1998 Krister Walfridsson
	*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "uae.h"
#include "options.h"
#include "memory-uae.h"
#include "custom.h"
#include "joystick.h"
#include "SDL.h"
#include "menu.h"
#include "menu_config.h"
#include "math.h"

#ifdef USE_UAE4ALL_VKBD
#include "vkbd.h"
static int can_change_vkbd_transparency=1;
#endif

#ifdef ANDROIDSDL
#include <android/log.h>
#endif

#ifdef GP2X
#include "gp2x.h"
#include "xwin.h"
extern int gp2xMouseEmuOn;
extern int gp2xButtonRemappingOn;
#if defined(PANDORA) || defined (ANDROIDSDL)
extern int dpadUp[4];
extern int dpadDown[4];
extern int dpadLeft[4];
extern int dpadRight[4];
extern int buttonA[4];
extern int buttonB[4];
extern int buttonX[4];
extern int buttonY[4];
extern int triggerL[4];
extern int triggerR[4];
extern int buttonSelect[4];
extern int buttonStart[4];
#endif
#ifdef __PSP2__
extern int rAnalogX;
extern int rAnalogY;
extern int lAnalogX;
extern int lAnalogY;
extern int mainMenu_leftStickMouse;
extern int mainMenu_touchControls;
extern int mainMenu_deadZone;
int delay2[] = {0, 0, 0}; // for 2nd, 3rd and 4th player non-custom autofire
#endif

extern char launchDir[300];
bool switch_autofire=false;
int delay=0;
#endif

int nr_joysticks;
bool parport_joystick_enabled = true;
//needed for parallel port joystick handling done in this file
extern int joy2_dir, joy3_dir;
extern int joy2_button, joy3_button;

SDL_Joystick *uae4all_joy0, *uae4all_joy1, *uae4all_joy2, *uae4all_joy3;
extern SDL_Surface *prSDLScreen;

void read_joystick(int nr, unsigned int *dir, int *button)
{
#ifndef MAX_AUTOEVENTS
		int x_axis, y_axis;
		int left = 0, right = 0, top = 0, bot = 0, upRight=0, downRight=0, upLeft=0, downLeft=0, x=0, y=0, a=0, b=0;
		int len, i, num;

		SDL_Joystick *joy;
		switch (nr)
		{
			case 0:
				joy = uae4all_joy0;
				break;
			case 1:
				joy = uae4all_joy1;
				break;
			case 2:
				joy = uae4all_joy2;
				break;
			case 3:
				joy = uae4all_joy3;
				break;
			default:
				joy = uae4all_joy1;
				break;
		}

		*dir = 0;
		*button = 0;

		if (nr == 1)
			nr = 0;
		else if (nr == 0)
			nr 1;

	// are we trying to figure out the regular GP2X controls for the primary (or first two) joysticks?
	int usingRegularControls = (!mainMenu_customControls) && ((mainMenu_joyPort == 0 && (nr == 0 || nr == 1)) || (nr == 1 && mainMenu_joyPort == 2) || (nr == 0 && mainMenu_joyPort == 1));
	
	//PSP2 updates joysticks in handle_events function which is always called 
	//just before read_joystick is called. No need to update them again here
#ifndef __PSP2__
	SDL_JoystickUpdate();
#endif
/* Temporary disabled
#ifdef ANDROIDSDL
		if (nr_joysticks > 2)
		{
	int axis0,axis1,axis2,axis3;
	axis0 = SDL_JoystickGetAxis(uae4all_joy2, 0);
	axis1 = SDL_JoystickGetAxis(uae4all_joy2, 1);
	axis2 = SDL_JoystickGetAxis(uae4all_joy2, 2);
	axis3 = SDL_JoystickGetAxis(uae4all_joy2, 3);

	if ((axis0 > 512) || (axis2 > 512)) right=1;
	if ((axis0 < -512) || (axis2 < -512)) left=1;
	if ((axis1 < -512) || (axis3 < -512)) top=1;
	if ((axis1 > 512) || (axis3 > 512)) bot=1;
}
#endif
*/
	int mouseScale = mainMenu_mouseMultiplier * 8 * 16;
	mouseScale /= 100;
	//slow down mouse motion if custom "slow mouse" button is held
	if(mainMenu_customControls)
	{
		for (int i=0; i<nr_joysticks; i++)
		{
			if( 
				(
					(mainMenu_custom_A[i]==-25 && buttonA[i]) ||
					(mainMenu_custom_B[i]==-25 && buttonB[i]) ||
					(mainMenu_custom_X[i]==-25 && buttonX[i]) ||
					(mainMenu_custom_Y[i]==-25 && buttonY[i]) ||
					(mainMenu_custom_L[i]==-25 && triggerL[i]) ||
					(mainMenu_custom_R[i]==-25 && triggerR[i])
				) ||
				(
					(mainMenu_custom_dpad == 0) && 
					(
						(mainMenu_custom_up[i]==-25 && dpadUp[i]) ||
						(mainMenu_custom_down[i]==-25 && dpadDown[i]) ||
						(mainMenu_custom_left[i]==-25 && dpadLeft[i]) ||
						(mainMenu_custom_right[i]==-25 && dpadRight[i]))
					)
				)
			{
				mouseScale/=8;
				break;
			}
		}
	}

//Digital mouseemu hotkeys: Triangle changes mouse speed etc.
#if !defined(__PSP2__) && defined(USE_UAE4ALL_VKBD)
	if (!vkbd_mode && ((mainMenu_customControls && mainMenu_custom_dpad==2) || gp2xMouseEmuOn || (triggerL[0] && !triggerR[0] && !gp2xButtonRemappingOn)))
#else
#if defined(__PSP2__) && defined(USE_UAE4ALL_VKBD)
	//on Vita, the L trigger is by default mapped to a mousebutton
	//so remove the hard coded LTrigger here that was enabling the digital mouse
	if (!vkbd_mode && ((mainMenu_customControls && mainMenu_custom_dpad==2) || gp2xMouseEmuOn))
#else
#if defined(__PSP2__)
	if ((mainMenu_customControls && mainMenu_custom_dpad==2) || gp2xMouseEmuOn)
#else
	if (((mainMenu_customControls && mainMenu_custom_dpad==2) || gp2xMouseEmuOn || (triggerL[0] && !triggerR[0] && !gp2xButtonRemappingOn)))
#endif
#endif
#endif
	{
		if (buttonY[0]) {
			mouseScale = mainMenu_mouseMultiplier * 16;
			mouseScale /= 100;
		}
#if defined(PANDORA) || defined(ANDROIDSDL)
		if (dpadLeft[0])
#else
		if (SDL_JoystickGetButton(joy, GP2X_BUTTON_LEFT))
#endif
		{
			lastmx -= mouseScale;
			newmousecounters=1;
		}
#if !( defined(PANDORA) || defined(ANDROIDSDL) )
		if (SDL_JoystickGetButton(joy, GP2X_BUTTON_UPLEFT))
		{
			lastmx -= mouseScale;
			lastmy -= mouseScale;
			newmousecounters=1;
		}
		if (SDL_JoystickGetButton(joy, GP2X_BUTTON_DOWNLEFT))
		{
			lastmx -= mouseScale;
			lastmy += mouseScale;
			newmousecounters=1;
		}
#endif
		if (dpadRight[0])
		{
			lastmx += mouseScale;
			newmousecounters=1;
		}
		if (dpadUp[0])
		{
			lastmy -= mouseScale;
			newmousecounters=1;
		}
		if (dpadDown[0])
		{
			lastmy += mouseScale;
			newmousecounters=1;
		}
	}
	else if (!triggerR[0] /*R+dpad = arrow keys*/ && !(mainMenu_customControls && mainMenu_custom_dpad==0) && usingRegularControls)
	{
//regular direction controls for main Joystick (or both if "both" is set.)
#if !defined(AROS) && !defined(__PSP2__)
		if (dpadRight || SDL_JoystickGetAxis(joy, 0) > 0) right=1;
		if (dpadLeft || SDL_JoystickGetAxis(joy, 0) < 0) left=1;
		if (dpadUp || SDL_JoystickGetAxis(joy, 1) < 0) top=1;
		if (dpadDown || SDL_JoystickGetAxis(joy, 1) > 0) bot=1;
#else
		if (dpadRight[0]) right=1;
		if (dpadLeft[0]) left=1;
		if (dpadUp[0]) top=1;
		if (dpadDown[0]) bot=1;
#endif

		if (mainMenu_joyConf)
		{
#ifdef USE_UAE4ALL_VKBD
			if (((buttonX[0] && mainMenu_jump > -1) ) && !vkbd_mode)
#else
			if (((buttonX[0] && mainMenu_jump > -1) || SDL_JoystickGetButton(joy, mainMenu_jump)))
#endif
				top = 1;
		}
	}

// regular button controls without custom remapping
#ifdef USE_UAE4ALL_VKBD
	if (usingRegularControls && !(gp2xMouseEmuOn) && !(gp2xButtonRemappingOn) && !vkbd_mode)
#else
	if (usingRegularControls && !(gp2xMouseEmuOn) && !(gp2xButtonRemappingOn))
#endif
	{
		if (
			(mainMenu_autofire & switch_autofire & delay>mainMenu_autofireRate)
			||
				(
					(
						(mainMenu_autofireButton1==GP2X_BUTTON_B && buttonA[0])
						||
						(mainMenu_autofireButton1==GP2X_BUTTON_X && buttonX[0])
						||
						(mainMenu_autofireButton1==GP2X_BUTTON_Y && buttonY[0])
					)
					& delay>mainMenu_autofireRate
				)
			)
		{
			if(!buttonB[0])
				*button=1;
			delay=0;
			*button |= (buttonB[0] & 1) << 1;
		}
		else
		{
#ifdef __PSP2__
			*button = ((mainMenu_button1==GP2X_BUTTON_B && buttonA[0]) || (mainMenu_button1==GP2X_BUTTON_X && buttonX[0]) || (mainMenu_button1==GP2X_BUTTON_Y && buttonY[0])) & 1;
#else
#if !(defined(ANDROIDSDL) || defined(AROS))
			*button = ((mainMenu_button1==GP2X_BUTTON_B && buttonA[0]) || (mainMenu_button1==GP2X_BUTTON_X && buttonX[0]) || (mainMenu_button1==GP2X_BUTTON_Y && buttonY[0]) || SDL_JoystickGetButton(joy, mainMenu_button1)) & 1;
#else
			*button = ((mainMenu_button1==GP2X_BUTTON_B && buttonA[0]) || (mainMenu_button1==GP2X_BUTTON_X && buttonX[0]) || (mainMenu_button1==GP2X_BUTTON_Y && buttonY[0])) & 1;
#endif
#endif //__PSP2__
			delay++;
#ifdef __PSP2__
			*button |= ((buttonB[0]) & 1) << 1;
#else
#if defined(PANDORA) && !defined(AROS)
			*button |= ((buttonB[0] || SDL_JoystickGetButton(joy, mainMenu_button2)) & 1) << 1;
#else
			*button |= ((buttonB[0]) & 1) << 1;
#endif
#endif //__PSP2__
		}
	}

//Analog Mouse on PSP2, only update once per frame (when nr==1)
#ifdef __PSP2__
#ifdef USE_UAE4ALL_VKBD
	if (!vkbd_mode && (nr == 1)
#else
	if (nr == 1)
#endif
	{
		//VITA: always use an analog stick (default: right stick) for mouse pointer movements
		//here we are using a small deadzone
		//This can be disabled in the menu because it interferes with Joystick Port 0
		if (mainMenu_mouseEmulation)
		{
			float analogX=0.0f;
			float analogY=0.0f;
			float deadZone=(float) mainMenu_deadZone;
			float scalingFactor=1.0f;
			float magnitude=0.0f;

			if (mainMenu_leftStickMouse)
			{
				analogX=(float) lAnalogX;
				analogY=(float) lAnalogY;
			}
			else
			{
				analogX=(float) rAnalogX;
				analogY=(float) rAnalogY;
			}
			//radial and scaled deadzone
			//http://www.third-helix.com/2013/04/12/doing-thumbstick-dead-zones-right.html
			//max movement is mouseScale.
			//that way, when in one of the other mouse modes,
			//the Y button to change scale still works
			magnitude=sqrt(analogX*analogX+analogY*analogY);
			if (magnitude >= deadZone)
			{
				scalingFactor=1.0f/magnitude*(magnitude-deadZone)/(32769.0f-deadZone);
				analogX = analogX * scalingFactor;
				analogY = analogY * scalingFactor;
				lastmx += (int) (analogX * mouseScale);
				lastmy += (int) (analogY * mouseScale);
				newmousecounters=1;
			}
		}
		else
		{
			newmousecounters=0;
		}
	}
#endif //__PSP2__

#ifdef USE_UAE4ALL_VKBD
	if(mainMenu_customControls && !vkbd_mode)
#else
	if(mainMenu_customControls)
#endif
	{
		for (int i=0; i<nr_joysticks; i++)
		{
			// mapping directions
			int u, d, l, r;
			switch (nr)
			{
				case 0:
				//second (mouseport) joystick
					u = -9;
					d = -10;
					l = -11;
					r = -12;
					break;
				case 1:
				//first (joyport) joystick
					u = -5;
					d = -6;
					l = -7;
					r = -8;
					break;
				case 2:
				//third (parallel port) joystick
					u = -15;
					d = -16;
					l = -17;
					r = -18;
					break;
				case 3:
				//fourth (parallel port) joystick
					u = -21;
					d = -22;
					l = -23;
					r = -24;
					break;
				default:
					break;
			}
			if((mainMenu_custom_A[i]==u && buttonA[i]) || (mainMenu_custom_B[i]==u && buttonB[i]) || (mainMenu_custom_X[i]==u && buttonX[i]) || (mainMenu_custom_Y[i]==u && buttonY[i]) || (mainMenu_custom_L[i]==u && triggerL[i]) || (mainMenu_custom_R[i]==u && triggerR[i]))
				top = 1;
			else if(mainMenu_custom_dpad == 0)
			{
				if((mainMenu_custom_up[i]==u && dpadUp[i]) || (mainMenu_custom_down[i]==u && dpadDown[i]) || (mainMenu_custom_left[i]==u && dpadLeft[i]) || (mainMenu_custom_right[i]==u && dpadRight[i]))
					top = 1;
			}

			if((mainMenu_custom_A[i]==d && buttonA[i]) || (mainMenu_custom_B[i]==d && buttonB[i]) || (mainMenu_custom_X[i]==d && buttonX[i]) || (mainMenu_custom_Y[i]==d && buttonY[i]) || (mainMenu_custom_L[i]==d && triggerL[i]) || (mainMenu_custom_R[i]==d && triggerR[i]))
				bot = 1;
			else if(mainMenu_custom_dpad == 0)
			{
				if((mainMenu_custom_up[i]==d && dpadUp[i]) || (mainMenu_custom_down[i]==d && dpadDown[i]) || (mainMenu_custom_left[i]==d && dpadLeft[i]) || (mainMenu_custom_right[i]==d && dpadRight[i]))
					bot = 1;
			}

			if((mainMenu_custom_A[i]==l && buttonA[i]) || (mainMenu_custom_B[i]==l && buttonB[i]) || (mainMenu_custom_X[i]==l && buttonX[i]) || (mainMenu_custom_Y[i]==l && buttonY[i]) || (mainMenu_custom_L[i]==l && triggerL[i]) || (mainMenu_custom_R[i]==l && triggerR[i]))
				left = 1;
			else if(mainMenu_custom_dpad == 0)
			{
				if((mainMenu_custom_up[i]==l && dpadUp[i]) || (mainMenu_custom_down[i]==l && dpadDown[i]) || (mainMenu_custom_left[i]==l && dpadLeft[i]) || (mainMenu_custom_right[i]==l && dpadRight[i]))
					left = 1;
			}

			if((mainMenu_custom_A[i]==r && buttonA[i]) || (mainMenu_custom_B[i]==r && buttonB[i]) || (mainMenu_custom_X[i]==r && buttonX[i]) || (mainMenu_custom_Y[i]==r && buttonY[i]) || (mainMenu_custom_L[i]==r && triggerL[i]) || (mainMenu_custom_R[i]==r && triggerR[i]))
				right = 1;
			else if(mainMenu_custom_dpad == 0)
			{
				if((mainMenu_custom_up[i]==r && dpadUp[i]) || (mainMenu_custom_down[i]==r && dpadDown[i]) || (mainMenu_custom_left[i]==r && dpadLeft[i]) || (mainMenu_custom_right[i]==r && dpadRight[i]))
					right = 1;
			}
		}
	}

	if(!gp2xMouseEmuOn && !gp2xButtonRemappingOn)
	{
#ifdef USE_UAE4ALL_VKBD
		if(mainMenu_customControls && !vkbd_mode)
#else
		if(mainMenu_customControls)
#endif
		{
			for (int i=0; i<nr_joysticks; i++)
			{
				// mapping buttons
				int b1, b2;
				switch (nr)
				{
					case 0:
					//second (mouseport) joystick
						b1 = -1;
						b2 = -2;
						break;
					case 1:
					//first (joyport) joystick
						b1 = -3;
						b2 = -4;
						break;
					case 2:
					//third (parallel port) joystick
						b1 = -13;
						b2 = -14;
						break;
					case 3:
					//fourth (parallel port) joystick
						b1 = -19;
						b2 = -20;
						break;
					default:
						break;
				}
				//mapping buttons
				if((mainMenu_custom_A[i]==b1 && buttonA[i]) || (mainMenu_custom_B[i]==b1 && buttonB[i]) || (mainMenu_custom_X[i]==b1 && buttonX[i]) || (mainMenu_custom_Y[i]==b1 && buttonY[i]) || (mainMenu_custom_L[i]==b1 && triggerL[i]) || (mainMenu_custom_R[i]==b1 && triggerR[i]))
					*button = 1;
				else if(mainMenu_custom_dpad == 0)
				{
					if((mainMenu_custom_up[i]==b1 && dpadUp[i]) || (mainMenu_custom_down[i]==b1 && dpadDown[i]) || (mainMenu_custom_left[i]==b1 && dpadLeft[i]) || (mainMenu_custom_right[i]==b1 && dpadRight[i]))
						*button = 1;
				}
				if((mainMenu_custom_A[i]==b2 && buttonA[i]) || (mainMenu_custom_B[i]==b2 && buttonB[i]) || (mainMenu_custom_X[i]==b2 && buttonX[i]) || (mainMenu_custom_Y[i]==b2 && buttonY[i]) || (mainMenu_custom_L[i]==b2 && triggerL[i]) || (mainMenu_custom_R[i]==b2 && triggerR[i]))
					*button |= 1 << 1;
				else if(mainMenu_custom_dpad == 0)
				{
					if((mainMenu_custom_up[i]==b2 && dpadUp[i]) || (mainMenu_custom_down[i]==b2 && dpadDown[i]) || (mainMenu_custom_left[i]==b2 && dpadLeft[i]) || (mainMenu_custom_right[i]==b2 && dpadRight[i]))
						*button |= 1 << 1;
				}
			}
		}
	}

#ifdef USE_UAE4ALL_VKBD
	if (vkbd_mode && nr == 1)
	{
		// move around the virtual keyboard instead

		// if Start+Dpad is used (or right analog stick on Vita)
		// move the keyboard itself and/or change transparency
#ifdef __PSP2__
		if (rAnalogY < -1024*10)
#else
		if (buttonStart[0] && (dpadUp[0] || top))
#endif
		{
			vkbd_displace_up();
		}
#ifdef __PSP2__
		else if (rAnalogY > 1024*10)
#else
		else if (buttonStart[0] && (dpadDown[0] || bot))
#endif
		{
			vkbd_displace_down();
		}
#ifdef __PSP2__
		else if (rAnalogX < -1024*10)
#else
		else if (buttonStart[0] && (dpadLeft[0] || left))
#endif
		{
			if (can_change_vkbd_transparency)
			{
				vkbd_transparency_up();
				can_change_vkbd_transparency=0;
			}
		}
#ifdef __PSP2__
		else if (rAnalogX > 1024*10)
#else
		else if (buttonStart[0] && (dpadRight[0] || right))
#endif
		{
			if (can_change_vkbd_transparency)
			{
				vkbd_transparency_down();
				can_change_vkbd_transparency=0;
			}
		}
		else
		{
			can_change_vkbd_transparency=1;
			if (left || dpadLeft[0])
				vkbd_move |= VKBD_LEFT;
			else
			{
				vkbd_move &= ~VKBD_LEFT;
				if (right || dpadRight[0])
					vkbd_move |= VKBD_RIGHT;
				else
					vkbd_move &= ~VKBD_RIGHT;
			}
			if (top || dpadUp[0])
				vkbd_move |= VKBD_UP;
			else
			{
				vkbd_move &= ~VKBD_UP;
				if (bot || dpadDown[0])
					vkbd_move |= VKBD_DOWN;
				else
					vkbd_move &= ~VKBD_DOWN;
			}
#ifdef __PSP2__ //we know the Vita has many buttons available so use those
			if (buttonX[0])
			{
				vkbd_move=VKBD_BUTTON;
				buttonX[0] = 0;
				*button = 0;
			}
			else if (buttonY[0])
			{
				vkbd_move=VKBD_BUTTON_SHIFT;
				buttonY[0] = 0;
				*button = 0;
			}
			else if (buttonA[0])
			{
				vkbd_move=VKBD_BUTTON_BACKSPACE;
				buttonA[0] = 0;
				*button = 0;
			}
			else if (buttonB[0])
			{
				vkbd_move=VKBD_BUTTON_RESET_STICKY;
				buttonB[0] = 0;
				*button = 0;
			}
#else // in other cases where those buttons might not be available, use the amiga joystick
			if (*button || buttonX[0] )
			{
				vkbd_move=VKBD_BUTTON;
				buttonX[0] = 0;
				*button = 0;
			}
#endif
			else { //button release, pressing sticky keys is possible again.
				for (int i=0; i<NUM_STICKY; i++)
				{
					vkbd_sticky_key[i].can_switch=true;
				}
			}
			// TODO: add vkbd_button2 mapped to button2
		}
	}
	else
#endif
	{
#ifdef __PSP2__
		// On Vita, map the second player to always using the GP2X mapping, in addition to everything else
		// Unless there is a custom mapping
		if (!mainMenu_customControls && ((nr == 0 && mainMenu_joyPort == 2) || (nr == 1 && mainMenu_joyPort == 1) || nr == 2 || nr == 3))
		{
			int joynum = 1;
			if (nr == 2 || nr == 3)
				joynum = nr;
			if (dpadRight[joynum])
				right = 1;
			else if (dpadLeft[joynum])
				left = 1;
			if (dpadUp[joynum])
				top = 1;
			else if (dpadDown[joynum])
				bot = 1;
			if (
				(mainMenu_autofire & switch_autofire & delay2[joynum]>mainMenu_autofireRate)
				||
					(
						(
							(mainMenu_autofireButton1==GP2X_BUTTON_B && buttonA[joynum])
							||
							(mainMenu_autofireButton1==GP2X_BUTTON_X && buttonX[joynum])
							||
							(mainMenu_autofireButton1==GP2X_BUTTON_Y && buttonY[joynum])
						)
						& delay2[joynum]>mainMenu_autofireRate
					)
				)
			{
				if(!buttonB[joynum])
					*button=1;
				delay2[joynum]=0;
				*button |= (buttonB[joynum] & 1) << 1;
			}
			else
			{
				if ((mainMenu_button1==GP2X_BUTTON_B && buttonA[joynum]) || (mainMenu_button1==GP2X_BUTTON_X && buttonX[joynum]) || (mainMenu_button1==GP2X_BUTTON_Y && buttonY[joynum]))
					*button |= 0x01;
				if (buttonB[joynum])
					*button |= (0x01 << 1);
				delay2[joynum]++;
			}
		}
#endif //__PSP2__
		// normal joystick movement
		// make sure it is impossible to press left + right etc
		if (left)
			top = !top;
		if (right)
			bot = !bot;
		*dir = bot | (right << 1) | (top << 8) | (left << 9);
	}

#ifndef __PSP2__
//If not on Vita, zero the "other" joystick
	if (!mainMenu_customControls)
	{
		if(mainMenu_joyPort != 0)
		{
		 // Only one joystick active
			if((nr == 0 && mainMenu_joyPort == 2) || (nr == 1 && mainMenu_joyPort == 1))
			{
				*dir = 0;
				*button = 0;
			}
		}
	}


#endif //__PSP2__
#endif // MAXAUTOEVENTS
}

void init_joystick(void)
{
	int i;
	nr_joysticks = SDL_NumJoysticks();
	if (nr_joysticks > 0)
		uae4all_joy0 = SDL_JoystickOpen(0);
	if (nr_joysticks > 1)
		uae4all_joy1 = SDL_JoystickOpen(1);
	else
		uae4all_joy1 = NULL;
	if (nr_joysticks > 2)
		uae4all_joy2 = SDL_JoystickOpen(2);
	else
		uae4all_joy2 = NULL;
	if (nr_joysticks > 3)
		uae4all_joy3 = SDL_JoystickOpen(3);
	else
		uae4all_joy3 = NULL;
}

void close_joystick(void)
{
	if (nr_joysticks > 0)
		SDL_JoystickClose (uae4all_joy0);
	if (nr_joysticks > 1)
		SDL_JoystickClose (uae4all_joy1);
	if (nr_joysticks > 2)
		SDL_JoystickClose (uae4all_joy2);
	if (nr_joysticks > 3)
		SDL_JoystickClose (uae4all_joy3);
}
