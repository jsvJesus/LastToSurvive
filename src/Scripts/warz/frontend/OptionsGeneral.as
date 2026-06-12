package warz.frontend {
	import flash.display.MovieClip;
	import flash.events.Event;
	import flash.events.MouseEvent;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;
	import flash.geom.Point;
	
	public class OptionsGeneral extends MovieClip {
		public var isActiveScreen:Boolean = false;

		public var	OptionsGeneralClip:MovieClip;
		public var	api:warz.frontend.Frontend=null;

		public var Opt_ScreenResIndex :Object;
		public var Opt_OverallQ :Object;
		public var Opt_Brightness :Object;
		//public var Opt_Contrast :Object;
		public var Opt_SoundV :Object;
		public var Opt_MusicV :Object;
		public var Opt_hintS :Object;
		public var Opt_RedBlood :Object;
		public var Opt_AA :Object;
		public var Opt_VSync :Object;
		public var Opt_Fullscreen :Object;
		public var Opt_SSAO :Object;
		public var Opt_TerrainD :Object;
		public var Opt_TerrainT :Object;
		public var Opt_WaterQ :Object;
		public var Opt_ShadowQ :Object;
		public var Opt_LightQ :Object;
		public var Opt_EffectsQ :Object;
		public var Opt_MeshD :Object;
		//public var Opt_Aniso :Object;
		public var Opt_PostProc :Object;
		public var Opt_MeshT :Object;

		public var arLMH :Array; // low, medium, high
		public var arLMHU :Array; // low, medium, high, ultra
		public var arLMHUC :Array; // low, medium high, ultra, custom
		public var arNY :Array; // No, Yes
		public var arLH :Array; // Low, High
		public var arOLMHU :Array; // Off, Low, Medium, High, Ultra
		public var arVLMHU :Array; // Very Low, Low, Medium, High, Ultra
		public var arOLMH :Array; // Off, Low, Medium, High
		public var arCenOffCen :Array; // center, off center
		
		public	var	isMouseDown:Boolean;
		public	var	downTarget:MovieClip;

		public	function OptionsGeneral ()
		{
			OptionsGeneralClip.General_Options.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsGeneralClip.General_Options.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsGeneralClip.General_Options.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsGeneralClip.Controls.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsGeneralClip.Controls.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsGeneralClip.Controls.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsGeneralClip.Language.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsGeneralClip.Language.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsGeneralClip.Language.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);

			OptionsGeneralClip.Voip.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsGeneralClip.Voip.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsGeneralClip.Voip.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);

			OptionsGeneralClip.BtnReset.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsGeneralClip.BtnReset.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsGeneralClip.BtnReset.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsGeneralClip.BtnApply.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsGeneralClip.BtnApply.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsGeneralClip.BtnApply.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
	
			OptionsGeneralClip.BtnReset.Text.Text.text = "$FR_Reset";
			OptionsGeneralClip.BtnApply.Text.Text.text = "$FR_Apply";
			
			Opt_ScreenResIndex = new Object();
			Opt_OverallQ = new Object();
			Opt_Brightness = new Object();
			//Opt_Contrast = new Object();
			Opt_SoundV = new Object();
			Opt_MusicV = new Object();
			Opt_hintS = new Object();
			Opt_RedBlood = new Object();
			Opt_AA = new Object();
			Opt_VSync = new Object();
			Opt_Fullscreen = new Object();
			Opt_SSAO = new Object();
			Opt_TerrainD = new Object();
			Opt_TerrainT = new Object();
			Opt_WaterQ = new Object();
			Opt_ShadowQ = new Object();
			Opt_LightQ = new Object();
			Opt_EffectsQ = new Object();
			Opt_MeshD = new Object();
			//Opt_Aniso = new Object();
			Opt_PostProc = new Object();
			Opt_MeshT = new Object();
			
			arLMH = new Array("$FR_optLow", "$FR_optMed", "$FR_optHig");
			arLMHU = new Array("$FR_optLow", "$FR_optMed", "$FR_optHig", "$FR_optUlt");
			arLMHUC = new Array("$FR_optLow", "$FR_optMed", "$FR_optHig", "$FR_optUlt", "$FR_optCus");
			arNY = new Array("$FR_optNo", "$FR_optYes"); 
			arLH = new Array("$FR_optLow", "$FR_optHig");
			arOLMHU = new Array("$FR_optOff", "$FR_optLow", "$FR_optMed", "$FR_optHig", "$FR_optUlt");
			arVLMHU = new Array("$FR_optVeryLow", "$FR_optLow", "$FR_optMed", "$FR_optHig", "$FR_optUlt");
			arOLMH = new Array("$FR_optOff", "$FR_optLow", "$FR_optMed", "$FR_optHig");
			arCenOffCen = new Array("$FR_AimPositionCenter", "$FR_AimPositionOffCenter");
			
			
			OptionsGeneralClip.Gameplay.visible = false;
			OptionsGeneralClip.OptStat8.visible = false;
			OptionsGeneralClip.OptStat9.visible = false;
		}
		
		public function Activate()
		{
			api = warz.frontend.Frontend.api;
			
			this.gotoAndPlay("start");
			
			resetValues();
			
			OptionsGeneralClip.General_Options.State = "active";
			if (OptionsGeneralClip.General_Options.currentLable != "pressed")
				OptionsGeneralClip.General_Options.gotoAndPlay("pressed");

			api.setButtonText (OptionsGeneralClip.General_Options.Text, "$FR_optGeneral");
			api.setButtonText (OptionsGeneralClip.Controls.Text, "$FR_optControls");
			api.setButtonText (OptionsGeneralClip.Language.Text, "$FR_optLanguage");
			api.setButtonText (OptionsGeneralClip.Voip.Text, "$FR_optVoip");
		}
		
		public function Deactivate()
		{
			this.gotoAndPlay("end");
		}
		
		private function SlotRollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		
		private function SlotRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		
		private function SlotPressFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
				
				var	p:MovieClip = evt.currentTarget.parent;
				
				while (!(p as OptionsGeneral))
				{
					p = p.parent as MovieClip;
				}
				
				SoundEvents.eventSoundPlay("menu_click");
				
				(p as OptionsGeneral).ActionFunction(evt.currentTarget.parent.name);
			}
		}
		
		public function ActionFunction (button:String)
		{
			if (button == "General_Options")
			{
//				api.showOptionsGeneral();
//				OptionsGeneralClip.General2General.State = "off";
//				OptionsGeneralClip.General2General.gotoAndPlay("out");
			}
			else if (button == "Controls")
			{
				api.showOptionsControls();

				OptionsGeneralClip.Controls.State = "off";
				OptionsGeneralClip.Controls.gotoAndPlay("out");
			}
			else if (button == "Language")
			{
				api.showOptionsLanguage();

				OptionsGeneralClip.Language.State = "off";
				OptionsGeneralClip.Language.gotoAndPlay("out");
			}
			else if (button == "Voip")
			{
				api.showOptionsVoip();

				OptionsGeneralClip.Voip.State = "off";
				OptionsGeneralClip.Voip.gotoAndPlay("out");
			}
			else if (button == "BtnApply")
			{
				OptionsGeneralClip.BtnApply.State = "off";
				OptionsGeneralClip.BtnApply.gotoAndPlay("out");
				
				api.showInfoOkCancelMsg("$FR_ApplySettingAreYouSure", "", ApplyBtnCallBack); 
			}
			else if (button == "BtnReset")
			{
				OptionsGeneralClip.BtnReset.State = "off";
				OptionsGeneralClip.BtnReset.gotoAndPlay("out");
				
				api.showInfoOkCancelMsg("$FR_ResetSettingAreYouSure", "", function(isReset:Boolean){
										if(isReset)FrontEndEvents.eventOptionsReset();});
										
			}
		}
		
		
		public function ApplyBtnCallBack(isApply:Boolean):void{
			if(isApply){
				Opt_Brightness.myValue = getSliderValue2(OptionsGeneralClip.OptStat3);
				//Opt_Contrast.myValue = getSliderValue2(OptionsGeneralClip.OptStat4);
	
				Opt_SoundV.myValue = getSliderValue(OptionsGeneralClip.OptStat5);
				Opt_MusicV.myValue = getSliderValue(OptionsGeneralClip.OptStat6);
				//Opt_CommV.myValue = getSliderValue(OptionsGeneralClip.OptStat7);

				FrontEndEvents.eventOptionsApply(api.Opt_ScreenResolutions[Opt_ScreenResIndex.myValue],
				Opt_OverallQ.myValue+1, Opt_Brightness.myValue, 0/*Opt_Contrast.myValue*/, Opt_SoundV.myValue,
				Opt_MusicV.myValue, 0, Opt_hintS.myValue, Opt_RedBlood.myValue, 
				Opt_AA.myValue+1, Opt_SSAO.myValue+1, Opt_TerrainD.myValue+1, Opt_TerrainT.myValue+1,
				Opt_WaterQ.myValue+1, Opt_ShadowQ.myValue+1, Opt_LightQ.myValue+2, Opt_EffectsQ.myValue+1,
				Opt_MeshD.myValue+1, /*me.Opt_Aniso.myValue+1*/1, Opt_PostProc.myValue+1, Opt_MeshT.myValue+1,
				Opt_Fullscreen.myValue, Opt_VSync.myValue+1); 
			}
		}

		private function arrowRollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		
		private function arrowRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		
		private function onArrowClick(incr:Number, variable:Object, myArray:Array, Arrow:MovieClip)
		{
			var	p:MovieClip = (Arrow.parent as MovieClip);
			if(p.State == "disabled")
				return;
				
			var myVar:Number = variable.myValue;
			var prevMode:Number = myVar;
			myVar += incr;
			if(myVar<0)
				myVar = myArray.length-1;
			if(myVar>=myArray.length)
				myVar = 0;
	
			p.Value.Value1 = myArray[prevMode];
			p.Value.Value2 = myArray[myVar];
//			p.Value.Text.text = myArray[myVar];
			p.Value.gotoAndPlay ("start");
			variable.myValue = myVar;
		}
				
		private function arrowPressFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
//				evt.currentTarget.parent.State = "active";
//				evt.currentTarget.parent.gotoAndPlay("pressed");
				
				evt.currentTarget.parent.ActionFunction ();
			}
		}
		
		public	function resetValues ()
		{
			var	me = this;
			
			api = warz.frontend.Frontend.api;
			
			Opt_ScreenResIndex.myValue = api.Opt_ScreenResIndex;
			Opt_OverallQ.myValue = api.Opt_OverallQ-1;
			Opt_Brightness.myValue = api.Opt_Brightness;
			//Opt_Contrast.myValue = api.Opt_Contrast;
			Opt_SoundV.myValue = api.Opt_SoundV;
			Opt_MusicV.myValue = api.Opt_MusicV;
			//Opt_CommV.myValue = api.Opt_CommV;
			Opt_hintS.myValue = api.Opt_hintS;
			Opt_RedBlood.myValue = api.Opt_RedBlood;
			Opt_Fullscreen.myValue = api.Opt_Fullscreen;
			Opt_AA.myValue = api.Opt_AA-1;
			Opt_VSync.myValue = api.Opt_VSync-1;
			Opt_SSAO.myValue = api.Opt_SSAO-1;
			Opt_TerrainD.myValue = api.Opt_TerrainD-1;
			Opt_TerrainT.myValue = api.Opt_TerrainT-1;
			Opt_WaterQ.myValue = api.Opt_WaterQ-1;
			Opt_ShadowQ.myValue = api.Opt_ShadowQ-1;
			Opt_LightQ.myValue = api.Opt_LightQ-2;
			Opt_EffectsQ.myValue = api.Opt_EffectsQ-1;
			Opt_MeshD.myValue = api.Opt_MeshD-1;
			//Opt_Aniso.myValue = _root.api.Opt_Aniso-1;
			Opt_PostProc.myValue = api.Opt_PostProc-1;
			Opt_MeshT.myValue = api.Opt_MeshT-1;

// resolution			
			OptionsGeneralClip.OptStat1.Name.text = "$FR_optResolution";
			OptionsGeneralClip.OptStat1.Value.Text.text = api.Opt_ScreenResolutions[Opt_ScreenResIndex.myValue];
			OptionsGeneralClip.OptStat1.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat1.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat1.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat1.ArrowLeft.ActionFunction = function ()
			{
				me.onArrowClick(-1, me.Opt_ScreenResIndex, me.api.Opt_ScreenResolutions, this);
			}

			OptionsGeneralClip.OptStat1.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat1.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat1.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat1.ArrowRight.ActionFunction = function ()
			{
				me.onArrowClick(+1, me.Opt_ScreenResIndex, me.api.Opt_ScreenResolutions, this);
			}

// quality
//			OptStat2
			OptionsGeneralClip.OptStat2.Name.text = "$FR_optOverallQuality";
			OptionsGeneralClip.OptStat2.Value.Text.text = arLMHUC[Opt_OverallQ.myValue];

			OptionsGeneralClip.OptStat2.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat2.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat2.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat2.ArrowLeft.ActionFunction = function ()
			{
				me.onArrowClick(-1, me.Opt_OverallQ, me.arLMHUC, this);
				processOveralQualitySelection ();
			}

			OptionsGeneralClip.OptStat2.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat2.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat2.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat2.ArrowRight.ActionFunction = function ()
			{
				me.onArrowClick(+1, me.Opt_OverallQ, me.arLMHUC, this);
				processOveralQualitySelection ();
			}

// brightness
//			OptStat3
			OptionsGeneralClip.OptStat3.Name.text = "$FR_optBrightness";
			OptionsGeneralClip.OptStat3.Bar.Scale.scaleX = 0.99; // api.Opt_Brightness;

// contrast
//			OptStat4
			//OptionsGeneralClip.OptStat4.Name.text = "$FR_optContrast";
			//OptionsGeneralClip.OptStat4.Bar.Scale.scaleX = 0.99; // api.Opt_Contrast;

// sound Volume
//			OptStat5
			OptionsGeneralClip.OptStat5.Name.text = "$FR_optSoundVolume";
			OptionsGeneralClip.OptStat5.Bar.Scale.scaleX = 0.99; // api.Opt_SoundV;

// Music Volume
//			OptStat6
			OptionsGeneralClip.OptStat6.Name.text = "$FR_optMusicVolume";
			OptionsGeneralClip.OptStat6.Bar.Scale.scaleX = 0.99; // api.Opt_MusicV;

// comms Volume
//			OptStat7
			OptionsGeneralClip.OptStat7.visible = false;
			//OptionsGeneralClip.OptStat7.Name.text = "$FR_optCommVolume";
			//OptionsGeneralClip.OptStat7.Bar.Scale.scaleX = 0.99; // api.Opt_CommV;

// TPS Aim Position
//			OptStat8
			OptionsGeneralClip.OptStat8.Name.text = "$FR_AimPositionCenter";
			OptionsGeneralClip.OptStat8.Value.Text.text = arCenOffCen[Opt_hintS.myValue];
			OptionsGeneralClip.OptStat8.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat8.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat8.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat8.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat8.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat8.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat8.ArrowLeft.ActionFunction = function ()
			{
				me.onArrowClick(-1, me.Opt_hintS, me.arCenOffCen, this);
			}
			OptionsGeneralClip.OptStat8.ArrowRight.ActionFunction = function ()
			{
				me.onArrowClick(+1, me.Opt_hintS, me.arCenOffCen, this);
			}

// Voice Commands
//			OptStat9
			OptionsGeneralClip.OptStat9.Name.text = "$FR_optNoRedBlood";
			OptionsGeneralClip.OptStat9.Value.Text.text = arNY[Opt_RedBlood.myValue];
			OptionsGeneralClip.OptStat9.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat9.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat9.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat9.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat9.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat9.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat9.ArrowLeft.ActionFunction = function ()
			{
				me.onArrowClick(-1, me.Opt_RedBlood, me.arNY, this);
			}
			OptionsGeneralClip.OptStat9.ArrowRight.ActionFunction = function ()
			{
				me.onArrowClick(+1, me.Opt_RedBlood, me.arNY, this);
			}

			OptionsGeneralClip.OptStat10.Name.text = "$FR_optFullscreen";
			OptionsGeneralClip.OptStat10.Value.Text.text = arNY[Opt_Fullscreen.myValue];
//			OptionsGeneralClip.OptStat10.Value.gotoAndPlay("start");
			OptionsGeneralClip.OptStat10.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat10.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat10.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat10.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat10.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat10.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat10.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_Fullscreen, me.arNY, this); }
			OptionsGeneralClip.OptStat10.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_Fullscreen, me.arNY, this); }
				
			OptionsGeneralClip.OptStat11.Name.text = "$FR_optAntialiasing";
			OptionsGeneralClip.OptStat11.Value.Text.text = arNY[Opt_AA.myValue];
//			OptionsGeneralClip.OptStat11.Value.gotoAndPlay("start");
			OptionsGeneralClip.OptStat11.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat11.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat11.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat11.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat11.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat11.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat11.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_AA, me.arNY, this); }
			OptionsGeneralClip.OptStat11.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_AA, me.arNY, this); }
	
			OptionsGeneralClip.OptStat12.Name.text = "$FR_optVSync";
			OptionsGeneralClip.OptStat12.Value.Text.text = arNY[Opt_VSync.myValue];
//			OptionsGeneralClip.OptStat12.Value.gotoAndPlay("start");
			OptionsGeneralClip.OptStat12.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat12.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat12.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat12.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat12.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat12.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat12.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_VSync, me.arNY, this); }
			OptionsGeneralClip.OptStat12.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_VSync, me.arNY, this); }
	
			OptionsGeneralClip.OptStat13.Name.text = "$FR_optSSAO";
			OptionsGeneralClip.OptStat13.Value.Text.text = arOLMH[Opt_SSAO.myValue];
//			OptionsGeneralClip.OptStat13.Value.gotoAndPlay("start");
			OptionsGeneralClip.OptStat13.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat13.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat13.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat13.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat13.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat13.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat13.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_SSAO, me.arOLMH, this); }
			OptionsGeneralClip.OptStat13.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_SSAO, me.arOLMH, this); }
	
			OptionsGeneralClip.OptStat14.Name.text = "$FR_optTerrDetail";
			OptionsGeneralClip.OptStat14.Value.Text.text = arLMH[Opt_TerrainD.myValue];
//			OptionsGeneralClip.OptStat14.Value.gotoAndPlay("start");
			OptionsGeneralClip.OptStat14.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat14.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat14.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat14.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat14.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat14.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat14.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_TerrainD, me.arLMH, this); }
			OptionsGeneralClip.OptStat14.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_TerrainD, me.arLMH, this); }
	
			OptionsGeneralClip.OptStat15.Name.text = "$FR_optTerrTex";
			OptionsGeneralClip.OptStat15.Value.Text.text = arLMH[Opt_TerrainT.myValue];
//			OptionsGeneralClip.OptStat15.Value.gotoAndPlay("start");
			OptionsGeneralClip.OptStat15.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat15.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat15.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat15.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat15.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat15.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat15.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_TerrainT, me.arLMH, this); }
			OptionsGeneralClip.OptStat15.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_TerrainT, me.arLMH, this); }
	
			OptionsGeneralClip.OptStat17.Name.text = "$FR_optShadowQual";
			OptionsGeneralClip.OptStat17.Value.Text.text = arOLMHU[Opt_ShadowQ.myValue];
//			OptionsGeneralClip.OptStat17.Value.gotoAndPlay("start");
			OptionsGeneralClip.OptStat17.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat17.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat17.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat17.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat17.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat17.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat17.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_ShadowQ, me.arOLMHU, this); }
			OptionsGeneralClip.OptStat17.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_ShadowQ, me.arOLMHU, this); }
	
			OptionsGeneralClip.OptStat18.Name.text = "$FR_optLightQual";
			OptionsGeneralClip.OptStat18.Value.Text.text = arLH[Opt_LightQ.myValue];
//			OptionsGeneralClip.OptStat18.Value.gotoAndPlay("start");
			OptionsGeneralClip.OptStat18.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat18.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat18.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat18.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat18.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat18.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat18.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_LightQ, me.arLH, this); }
			OptionsGeneralClip.OptStat18.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_LightQ, me.arLH, this); }
	
			OptionsGeneralClip.OptStat20.Name.text = "$FR_optMeshQual";
			OptionsGeneralClip.OptStat20.Value.Text.text = arLMH[Opt_MeshD.myValue];
//			OptionsGeneralClip.OptStat20.Value.gotoAndPlay("start");
			OptionsGeneralClip.OptStat20.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat20.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat20.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat20.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat20.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat20.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat20.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_MeshD, me.arLMH, this); }
			OptionsGeneralClip.OptStat20.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_MeshD, me.arLMH, this); }
	
			//Opt.Anisotropy.Name.text = "$FR_optAniso";
			//Opt.Anisotropy.Value.Value2 = arOLMHU[Opt_Aniso.myValue];
			//Opt.Anisotropy.Value.gotoAndPlay("start");
			//Opt.Anisotropy.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_Aniso, me.arOLMHU, this); }
			//Opt.Anisotropy.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_Aniso, me.arOLMHU, this); }
	
			OptionsGeneralClip.OptStat21.Name.text = "$FR_optPostProcessQual";
			OptionsGeneralClip.OptStat21.Value.Text.text = arLMH[Opt_PostProc.myValue];
//			OptionsGeneralClip.OptStat21.Value.gotoAndPlay("start");
			OptionsGeneralClip.OptStat21.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat21.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat21.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat21.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat21.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat21.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat21.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_PostProc, me.arLMH, this); }
			OptionsGeneralClip.OptStat21.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_PostProc, me.arLMH, this); }
	
			OptionsGeneralClip.OptStat22.Name.text = "$FR_optTexturesQual";		
			OptionsGeneralClip.OptStat22.Value.Text.text = arLMH[Opt_MeshT.myValue];
//			OptionsGeneralClip.OptStat22.Value.gotoAndPlay("start");
			OptionsGeneralClip.OptStat22.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat22.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat22.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat22.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsGeneralClip.OptStat22.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsGeneralClip.OptStat22.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsGeneralClip.OptStat22.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_MeshT, me.arLMH, this); }
			OptionsGeneralClip.OptStat22.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_MeshT, me.arLMH, this); }

			setSliderValue2(OptionsGeneralClip.OptStat3, Opt_Brightness.myValue);
			//setSliderValue2(OptionsGeneralClip.OptStat4, Opt_Contrast.myValue);
	
			setSliderValue(OptionsGeneralClip.OptStat5, Opt_SoundV.myValue);
			setSliderValue(OptionsGeneralClip.OptStat6, Opt_MusicV.myValue);
			//setSliderValue(OptionsGeneralClip.OptStat7, Opt_CommV.myValue);
			
			OptionsGeneralClip.OptStat3.Bar.addEventListener(MouseEvent.MOUSE_DOWN, barPressFn2);
			//OptionsGeneralClip.OptStat4.Bar.addEventListener(MouseEvent.MOUSE_DOWN, barPressFn2);
			OptionsGeneralClip.OptStat5.Bar.addEventListener(MouseEvent.MOUSE_DOWN, barPressFn);
			OptionsGeneralClip.OptStat6.Bar.addEventListener(MouseEvent.MOUSE_DOWN, barPressFn);
			//OptionsGeneralClip.OptStat7.Bar.addEventListener(MouseEvent.MOUSE_DOWN, barPressFn);

			OptionsGeneralClip.OptStat3.Bar.addEventListener(MouseEvent.MOUSE_MOVE, barMoveFn2);
			//OptionsGeneralClip.OptStat4.Bar.addEventListener(MouseEvent.MOUSE_MOVE, barMoveFn2);
			OptionsGeneralClip.OptStat5.Bar.addEventListener(MouseEvent.MOUSE_MOVE, barMoveFn);
			OptionsGeneralClip.OptStat6.Bar.addEventListener(MouseEvent.MOUSE_MOVE, barMoveFn);
			//OptionsGeneralClip.OptStat7.Bar.addEventListener(MouseEvent.MOUSE_MOVE, barMoveFn);

			OptionsGeneralClip.OptStat3.Bar.addEventListener(MouseEvent.MOUSE_UP, barUpFn2);
			//OptionsGeneralClip.OptStat4.Bar.addEventListener(MouseEvent.MOUSE_UP, barUpFn2);
			OptionsGeneralClip.OptStat5.Bar.addEventListener(MouseEvent.MOUSE_UP, barUpFn);
			OptionsGeneralClip.OptStat6.Bar.addEventListener(MouseEvent.MOUSE_UP, barUpFn);
			//OptionsGeneralClip.OptStat7.Bar.addEventListener(MouseEvent.MOUSE_UP, barUpFn);

			stage.addEventListener(MouseEvent.MOUSE_UP, barUpFn);

			processOveralQualitySelection ();
		}
		
		private function barPressFn(evt:MouseEvent)
		{
			var	target = evt.currentTarget;
			var barW:Number = evt.currentTarget.width;
			var coord:Point = evt.currentTarget.globalToLocal(new Point(evt.stageX, evt.stageY));
			var xPos = (coord.x / barW);

			setSliderValue (evt.currentTarget.parent, xPos);
			isMouseDown = true;
			downTarget = target;
		}
		
		private function barMoveFn(evt:MouseEvent)
		{
			var	target = evt.currentTarget;
			
			if (isMouseDown && downTarget  == target)
			{
				var barW:Number = evt.currentTarget.width;
				var coord:Point = evt.currentTarget.globalToLocal(new Point(evt.stageX, evt.stageY));
				var xPos = (coord.x / barW);

				setSliderValue (evt.currentTarget.parent, xPos);
			}
		}
		
		private function barUpFn(evt:MouseEvent)
		{
			var	target = evt.currentTarget;
			
			isMouseDown = false;
			downTarget = null;
		}
		
		private function barPressFn2(evt:MouseEvent)
		{
			var	target = evt.currentTarget;
			var barW:Number = evt.currentTarget.width;
			var coord:Point = evt.currentTarget.globalToLocal(new Point(evt.stageX, evt.stageY));
			var xPos = 0.25 + (coord.x / barW) * 0.5;

			setSliderValue2 (evt.currentTarget.parent, xPos);
			isMouseDown = true;
			downTarget = target;
		}
		
		private function barMoveFn2(evt:MouseEvent)
		{
			var	target = evt.currentTarget;
			
			if (isMouseDown && downTarget  == target)
			{
				var barW:Number = evt.currentTarget.width;
				var coord:Point = evt.currentTarget.globalToLocal(new Point(evt.stageX, evt.stageY));
				var xPos = 0.25 + (coord.x / barW) * 0.5;

				setSliderValue2 (evt.currentTarget.parent, xPos);
			}
		}
		
		private function barUpFn2(evt:MouseEvent)
		{
			var	target = evt.currentTarget;
			
			isMouseDown = false;
			downTarget = null;
		}
		
		// param - from 0.0 to 1.0
		private function setSliderValue(control:MovieClip, param:Number)
		{
			param = Math.min (param, 1);
			param = Math.max (param, 0);	

			var barW:Number = 550;
			var	width:Number = param * barW;
			control.Bar.Scale.width = width;
		}
		
		private function getSliderValue(control:MovieClip):Number
		{
			var ret:Number = control.Bar.Scale.scaleX;
			if(ret<0) ret = 0;
			return ret;
		}
		
		// param - from 0.25 to 0.75
		private function setSliderValue2(control:MovieClip, param:Number)
		{
			param = (param - 0.25) / 0.5;
			param = Math.min (param, 1);
			param = Math.max (param, 0);	

			var barW:Number = 550;// control.Bar.width;
			var	width:Number = param * barW;
			control.Bar.Scale.width = width;
		}
		
		private function getSliderValue2(control:MovieClip):Number
		{
			var ret:Number = control.Bar.Scale.scaleX;
			
			ret = (ret * 0.5) + 0.25;
			ret = Math.min (ret, 0.75);
			ret = Math.max (ret, 0.25);	

			return ret;
		}
		
		private function processOveralQualitySelection()
		{
			if(Opt_OverallQ.myValue == 4)
			{
				OptionsGeneralClip.OptStat14.alpha = 1;
				OptionsGeneralClip.OptStat14.State = "none";
				OptionsGeneralClip.OptStat14.ArrowRight.useHandCursor = OptionsGeneralClip.OptStat14.ArrowLeft.useHandCursor = true;
	
				OptionsGeneralClip.OptStat15.alpha = 1;
				OptionsGeneralClip.OptStat15.State = "none";
				OptionsGeneralClip.OptStat15.ArrowRight.useHandCursor = OptionsGeneralClip.OptStat15.ArrowLeft.useHandCursor = true;
	
				OptionsGeneralClip.OptStat17.alpha = 1;
				OptionsGeneralClip.OptStat17.State = "none";
				OptionsGeneralClip.OptStat17.ArrowRight.useHandCursor = OptionsGeneralClip.OptStat17.ArrowLeft.useHandCursor = true;
	
				OptionsGeneralClip.OptStat18.alpha = 1;
				OptionsGeneralClip.OptStat18.State = "none";
				OptionsGeneralClip.OptStat18.ArrowRight.useHandCursor = OptionsGeneralClip.OptStat18.ArrowLeft.useHandCursor = true;
	
				OptionsGeneralClip.OptStat20.alpha = 1;
				OptionsGeneralClip.OptStat20.State = "none";
				OptionsGeneralClip.OptStat20.ArrowRight.useHandCursor = OptionsGeneralClip.OptStat20.ArrowLeft.useHandCursor = true;
	
				//Opt.Anisotropy.alpha = 100;
				//Opt.Anisotropy.State = "none";
				//Opt.Anisotropy.ArrowRight.useHandCursor = Opt.Anisotropy.ArrowLeft.useHandCursor = true;
	
				OptionsGeneralClip.OptStat21.alpha = 1;
				OptionsGeneralClip.OptStat21.State = "none";
				OptionsGeneralClip.OptStat21.ArrowRight.useHandCursor = OptionsGeneralClip.OptStat21.ArrowLeft.useHandCursor = true;
	
				OptionsGeneralClip.OptStat22.alpha = 1;
				OptionsGeneralClip.OptStat22.State = "none";
				OptionsGeneralClip.OptStat22.ArrowRight.useHandCursor = OptionsGeneralClip.OptStat22.ArrowLeft.useHandCursor = true;
			}
			else
			{
				OptionsGeneralClip.OptStat14.alpha = 0.5;
				OptionsGeneralClip.OptStat14.State = "disabled";
				OptionsGeneralClip.OptStat14.ArrowRight.useHandCursor = OptionsGeneralClip.OptStat14.ArrowLeft.useHandCursor = false;
	
				OptionsGeneralClip.OptStat15.alpha = 0.5;
				OptionsGeneralClip.OptStat15.State = "disabled";
				OptionsGeneralClip.OptStat15.ArrowRight.useHandCursor = OptionsGeneralClip.OptStat15.ArrowLeft.useHandCursor = false;
	
				OptionsGeneralClip.OptStat17.alpha = 0.5;
				OptionsGeneralClip.OptStat17.State = "disabled";
				OptionsGeneralClip.OptStat17.ArrowRight.useHandCursor = OptionsGeneralClip.OptStat17.ArrowLeft.useHandCursor = false;
	
				OptionsGeneralClip.OptStat18.alpha = 0.5;
				OptionsGeneralClip.OptStat18.State = "disabled";
				OptionsGeneralClip.OptStat18.ArrowRight.useHandCursor = OptionsGeneralClip.OptStat18.ArrowLeft.useHandCursor = false;
	
				OptionsGeneralClip.OptStat20.alpha = 0.5;
				OptionsGeneralClip.OptStat20.State = "disabled";
				OptionsGeneralClip.OptStat20.ArrowRight.useHandCursor = OptionsGeneralClip.OptStat20.ArrowLeft.useHandCursor = false;
	
				//Opt.Anisotropy.alpha = 50;
				//Opt.Anisotropy.State = "disabled";
				//Opt.Anisotropy.ArrowRight.useHandCursor = Opt.Anisotropy.ArrowLeft.useHandCursor = false;
	
				OptionsGeneralClip.OptStat21.alpha = 0.5;
				OptionsGeneralClip.OptStat21.State = "disabled";
				OptionsGeneralClip.OptStat21.ArrowRight.useHandCursor = OptionsGeneralClip.OptStat21.ArrowLeft.useHandCursor = false;
	
				OptionsGeneralClip.OptStat22.alpha = 0.5;
				OptionsGeneralClip.OptStat22.State = "disabled";
				OptionsGeneralClip.OptStat22.ArrowRight.useHandCursor = OptionsGeneralClip.OptStat22.ArrowLeft.useHandCursor = false;
			}
		}
	}
}
