package warz.frontend {
	import flash.display.MovieClip;
	import flash.events.*;
	import flash.utils.setInterval;
	import flash.utils.clearInterval;
	import warz.Utils;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;
	import warz.dataObjects.*;
	
	public class MainMenu extends MovieClip {
		public var MainMenuBtn:MovieClip;
		public var SurvivorsBtn:MovieClip;
		public var MarketplaceBtn:MovieClip;
		public var CommunityBtn:MovieClip;
		public var OptionsBtn:MovieClip;
		public var QuitBtn:MovieClip;
		
		private var BtnArray:Array = new Array;
		
		private var MainMenuBtnFX_Rot:Number;
		private var MainMenuBtnFX_Width:Number;
		private var MainMenuBtnFX_Height:Number;
		private var MainMenuBtnFX_Force:Boolean;

		public function MainMenu() {
			var me = this;
			BtnArray.push(MainMenuBtn);
			BtnArray.push(SurvivorsBtn);
			BtnArray.push(MarketplaceBtn);
			BtnArray.push(CommunityBtn);
			BtnArray.push(OptionsBtn);
			BtnArray.push(QuitBtn);
			
			// events
			MainMenuBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, RollOverBtnFn);
			MainMenuBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, RollOutBtnFn);
			MainMenuBtn.Btn.addEventListener(MouseEvent.CLICK, PressBtnFn);
			SurvivorsBtn.addEventListener(MouseEvent.MOUSE_OVER, RollOverBtnFn2);
			SurvivorsBtn.addEventListener(MouseEvent.MOUSE_OUT, RollOutBtnFn2);
			SurvivorsBtn.addEventListener(MouseEvent.CLICK, PressBtnFn2);
			MarketplaceBtn.addEventListener(MouseEvent.MOUSE_OVER, RollOverBtnFn2);
			MarketplaceBtn.addEventListener(MouseEvent.MOUSE_OUT, RollOutBtnFn2);
			MarketplaceBtn.addEventListener(MouseEvent.CLICK, PressBtnFn2);
			CommunityBtn.addEventListener(MouseEvent.MOUSE_OVER, RollOverBtnFn2);
			CommunityBtn.addEventListener(MouseEvent.MOUSE_OUT, RollOutBtnFn2);
			CommunityBtn.addEventListener(MouseEvent.CLICK, PressBtnFn2);
			OptionsBtn.addEventListener(MouseEvent.MOUSE_OVER, RollOverBtnFn2);
			OptionsBtn.addEventListener(MouseEvent.MOUSE_OUT, RollOutBtnFn2);
			OptionsBtn.addEventListener(MouseEvent.CLICK, PressBtnFn2);
			QuitBtn.addEventListener(MouseEvent.MOUSE_OVER, RollOverBtnFn2);
			QuitBtn.addEventListener(MouseEvent.MOUSE_OUT, RollOutBtnFn2);
			QuitBtn.addEventListener(MouseEvent.CLICK, PressBtnFn2);

			// event functions
			function RollOverBtnFn(evt:Event)
			{
				if(evt.currentTarget.parent.State != "active")
				{
					evt.currentTarget.parent.gotoAndPlay("over");
				}
			}
			function RollOutBtnFn(evt:Event)
			{
				if(evt.currentTarget.parent.State != "active")
				{
					evt.currentTarget.parent.gotoAndPlay("out");
				}
			}
			
			function PressBtnFn(evt:Event)
			{
				//if(evt.currentTarget.parent.State != "active")
				{
					for(var i=0; i<BtnArray.length; ++i)
					{
						if(BtnArray[i].State == "active")
						{
							BtnArray[i].State = "off";
							BtnArray[i].gotoAndPlay("out");
						}
					}
					
					SoundEvents.eventSoundPlay("menu_click");
					
					evt.currentTarget.parent.State = "active";
					evt.currentTarget.parent.gotoAndPlay("pressed");
					evt.currentTarget.parent.ActionFunction();
				}
			}
			function RollOverBtnFn2(evt:Event)
			{
				if(evt.currentTarget.State != "active")
				{
					evt.currentTarget.gotoAndPlay("over");
				}
			}
			function RollOutBtnFn2(evt:Event)
			{
				if(evt.currentTarget.State != "active")
				{
					evt.currentTarget.gotoAndPlay("out");
				}
			}
			
			function PressBtnFn2(evt:Event)
			{
				if(evt.currentTarget.State != "active")
				{
					for(var i=0; i<BtnArray.length; ++i)
					{
						if(BtnArray[i].State == "active")
						{
							BtnArray[i].State = "off";
							BtnArray[i].gotoAndPlay("out");
						}
					}
					
					SoundEvents.eventSoundPlay("menu_click");
					
					evt.currentTarget.State = "active";
					evt.currentTarget.gotoAndPlay("pressed");
					evt.currentTarget.ActionFunction();
				}
			}			
			MainMenuBtn.ActionFunction = function()
			{
				warz.frontend.Frontend.api.Main.showScreen("PlayGame");
			}
			
			SurvivorsBtn.ActionFunction = function()
			{
				warz.frontend.Frontend.api.Main.showScreen("Survivors");
			}
			
			MarketplaceBtn.ActionFunction = function()
			{
				warz.frontend.Frontend.api.Main.showScreen("MarketPlace");
			}
			
			CommunityBtn.ActionFunction = function()
			{
				warz.frontend.Frontend.api.Main.showScreen("Community");
			}
			
			OptionsBtn.ActionFunction = function()
			{
				warz.frontend.Frontend.api.Main.showScreen("OptionsGeneral");
			}
			
			QuitBtn.ActionFunction = function()
			{
				me.QuitBtn.gotoAndPlay("out");
				me.QuitBtn.State = "off";

				warz.frontend.Frontend.api.showInfoOkCancelMsg ("$FR_PAUSE_QUIT_SURE", "$FR_PAUSE_QUIT_GAME", quitGame);
			}
						
			MainMenuBtnFX_Rot = MainMenuBtn.FX.rotation;
			MainMenuBtnFX_Width = MainMenuBtn.FX.width;
			MainMenuBtnFX_Height = MainMenuBtn.FX.height;
			MainMenuBtnFX_Force = false;

			MainMenuBtn.addEventListener(Event.ENTER_FRAME, MainMenuBtnEnterFrame);
			function MainMenuBtnEnterFrame(evt:Event)
			{
				if(evt.currentTarget.currentFrame >=8 && evt.currentTarget.currentFrame <= 13)
				{
					if(evt.currentTarget.FXInterval == null || evt.currentTarget.FXInterval == undefined)
					{
						MainMenuBtnFX_Force = true;
						evt.currentTarget.FXInterval = setInterval(FXFunc, 200);
					}
				}
				else
				{
					if(evt.currentTarget.FXInterval != null)
					{
						clearInterval(evt.currentTarget.FXInterval);
						evt.currentTarget.FXInterval = null;
					}
				}
			}
			
			MainMenuBtn.TextFXInterval = setInterval(TextFXFunc, 50);
			
			function FXFunc()
			{
				var Trigger = Utils.randRange(0, 10);
				if(Trigger == 2 || MainMenuBtnFX_Force)
				{
					MainMenuBtn.FX.x = Utils.randRange(0, 200) + 150;
					MainMenuBtn.FX.y = Utils.randRange(0, 40) + 40;
					MainMenuBtn.Scale = Utils.randRange(0, 1.3) + 1;
					//trace(FX._width);
					MainMenuBtn.FX.width = MainMenuBtnFX_Width / MainMenuBtn.Scale;
					MainMenuBtn.FX.height = MainMenuBtnFX_Height / MainMenuBtn.Scale;
					MainMenuBtn.FX.rotation = MainMenuBtnFX_Rot+Utils.randRange(0, 360);
					MainMenuBtn.FX.gotoAndPlay("play");
					MainMenuBtnFX_Force = false;
				}
			}
			function TextFXFunc()
			{
				MainMenuBtn.TextFX.Text.alpha = Utils.randRange(0, 0.5);
			}
		}
		
		public	function popupPressed (e:Event):void
		{
			warz.frontend.Frontend.api.Main.PopUpAnim.PopUp.OK.removeEventListener(MouseEvent.CLICK, popupPressed);
			warz.frontend.Frontend.api.Main.PopUpAnim.gotoAndPlay("out");
		}
		
		public	function quitGame (state:Boolean):void
		{
			if (state)
				FrontEndEvents.eventQuitGame ();
		}
		
		public function updateTextLanguages()
		{
			if(warz.frontend.Frontend.api.m_Language == "english")
			{
				SurvivorsBtn.Text.gotoAndStop("Survivors");
				SurvivorsBtn.Add.gotoAndStop("Survivors");
				SurvivorsBtn.Line.gotoAndStop("long");
				
				MarketplaceBtn.Text.gotoAndStop("Marketplace");
				MarketplaceBtn.Add.gotoAndStop("Marketplace");
				MarketplaceBtn.Line.gotoAndStop("long");
	
				CommunityBtn.Text.gotoAndStop("Community");
				CommunityBtn.Add.gotoAndStop("Community");
				CommunityBtn.Line.gotoAndStop("long");
	
				OptionsBtn.Text.gotoAndStop("Options");
				OptionsBtn.Add.gotoAndStop("Options");
				OptionsBtn.Line.gotoAndStop("long");
	
				QuitBtn.Text.gotoAndStop("Quit");
				QuitBtn.Add.gotoAndStop("Quit");
				QuitBtn.Line.gotoAndStop("short");
				
				MainMenuBtn.TextFX.gotoAndStop("en");
				MainMenuBtn.Pressed.gotoAndStop("en");
				MainMenuBtn.Over.gotoAndStop("en");
				MainMenuBtn.Off.gotoAndStop("en");
			}
			else if(warz.frontend.Frontend.api.m_Language == "russian")
			{
				SurvivorsBtn.Text.gotoAndStop("Survivors_RU");
				SurvivorsBtn.Add.gotoAndStop("Survivors_RU");
				SurvivorsBtn.Line.gotoAndStop("long");
				
				MarketplaceBtn.Text.gotoAndStop("Marketplace_RU");
				MarketplaceBtn.Add.gotoAndStop("Marketplace_RU");
				MarketplaceBtn.Line.gotoAndStop("long");
	
				CommunityBtn.Text.gotoAndStop("Community_RU");
				CommunityBtn.Add.gotoAndStop("Community_RU");
				CommunityBtn.Line.gotoAndStop("long");
	
				OptionsBtn.Text.gotoAndStop("Options_RU");
				OptionsBtn.Add.gotoAndStop("Options_RU");
				OptionsBtn.Line.gotoAndStop("long");
	
				QuitBtn.Text.gotoAndStop("Quit_RU");
				QuitBtn.Add.gotoAndStop("Quit_RU");
				QuitBtn.Line.gotoAndStop("short");
				
				MainMenuBtn.TextFX.gotoAndStop("ru");
				MainMenuBtn.Pressed.gotoAndStop("ru");
				MainMenuBtn.Over.gotoAndStop("ru");
				MainMenuBtn.Off.gotoAndStop("ru");
			}
			else if(warz.frontend.Frontend.api.m_Language == "french")
			{
				SurvivorsBtn.Text.gotoAndStop("Survivors_FR");
				SurvivorsBtn.Add.gotoAndStop("Survivors_FR");
				SurvivorsBtn.Line.gotoAndStop("long");
				
				MarketplaceBtn.Text.gotoAndStop("Marketplace_FR");
				MarketplaceBtn.Add.gotoAndStop("Marketplace_FR");
				MarketplaceBtn.Line.gotoAndStop("long");
	
				CommunityBtn.Text.gotoAndStop("Community_FR");
				CommunityBtn.Add.gotoAndStop("Community_FR");
				CommunityBtn.Line.gotoAndStop("long");
	
				OptionsBtn.Text.gotoAndStop("Options_FR");
				OptionsBtn.Add.gotoAndStop("Options_FR");
				OptionsBtn.Line.gotoAndStop("long");
	
				QuitBtn.Text.gotoAndStop("Quit_FR");
				QuitBtn.Add.gotoAndStop("Quit_FR");
				QuitBtn.Line.gotoAndStop("short");
				
				MainMenuBtn.TextFX.gotoAndStop("fr");
				MainMenuBtn.Pressed.gotoAndStop("fr");
				MainMenuBtn.Over.gotoAndStop("fr");
				MainMenuBtn.Off.gotoAndStop("fr");
			}
			else if(warz.frontend.Frontend.api.m_Language == "german")
			{
				SurvivorsBtn.Text.gotoAndStop("Survivors_GE");
				SurvivorsBtn.Add.gotoAndStop("Survivors_GE");
				SurvivorsBtn.Line.gotoAndStop("long");
				
				MarketplaceBtn.Text.gotoAndStop("Marketplace_GE");
				MarketplaceBtn.Add.gotoAndStop("Marketplace_GE");
				MarketplaceBtn.Line.gotoAndStop("long");
	
				CommunityBtn.Text.gotoAndStop("Community_GE");
				CommunityBtn.Add.gotoAndStop("Community_GE");
				CommunityBtn.Line.gotoAndStop("long");
	
				OptionsBtn.Text.gotoAndStop("Options_GE");
				OptionsBtn.Add.gotoAndStop("Options_GE");
				OptionsBtn.Line.gotoAndStop("long");
				OptionsBtn.x = 1485.90 - 30;
	
				QuitBtn.Text.gotoAndStop("Quit_GE");
				QuitBtn.Add.gotoAndStop("Quit_GE");
				QuitBtn.Line.gotoAndStop("short");
				
				MainMenuBtn.TextFX.gotoAndStop("ge");
				MainMenuBtn.Pressed.gotoAndStop("ge");
				MainMenuBtn.Over.gotoAndStop("ge");
				MainMenuBtn.Off.gotoAndStop("ge");
			}
			else if(warz.frontend.Frontend.api.m_Language == "italian")
			{
				SurvivorsBtn.Text.gotoAndStop("Survivors_IT");
				SurvivorsBtn.Add.gotoAndStop("Survivors_IT");
				SurvivorsBtn.Line.gotoAndStop("long");
				
				MarketplaceBtn.Text.gotoAndStop("Marketplace_IT");
				MarketplaceBtn.Add.gotoAndStop("Marketplace_IT");
				MarketplaceBtn.Line.gotoAndStop("long");
	
				CommunityBtn.Text.gotoAndStop("Community_IT");
				CommunityBtn.Add.gotoAndStop("Community_IT");
				CommunityBtn.Line.gotoAndStop("long");
	
				OptionsBtn.Text.gotoAndStop("Options_IT");
				OptionsBtn.Add.gotoAndStop("Options_IT");
				OptionsBtn.Line.gotoAndStop("long");
	
				QuitBtn.Text.gotoAndStop("Quit_IT");
				QuitBtn.Add.gotoAndStop("Quit_IT");
				QuitBtn.Line.gotoAndStop("short");
				
				MainMenuBtn.TextFX.gotoAndStop("it");
				MainMenuBtn.Pressed.gotoAndStop("it");
				MainMenuBtn.Over.gotoAndStop("it");
				MainMenuBtn.Off.gotoAndStop("it");
			}
			else if(warz.frontend.Frontend.api.m_Language == "spanish")
			{
				SurvivorsBtn.Text.gotoAndStop("Survivors_SP");
				SurvivorsBtn.Add.gotoAndStop("Survivors_SP");
				SurvivorsBtn.Line.gotoAndStop("long");
				
				MarketplaceBtn.Text.gotoAndStop("Marketplace_SP");
				MarketplaceBtn.Add.gotoAndStop("Marketplace_SP");
				MarketplaceBtn.Line.gotoAndStop("long");
	
				CommunityBtn.Text.gotoAndStop("Community_SP");
				CommunityBtn.Add.gotoAndStop("Community_SP");
				CommunityBtn.Line.gotoAndStop("long");
				CommunityBtn.x = 1184.90 - 30;
	
				OptionsBtn.Text.gotoAndStop("Options_SP");
				OptionsBtn.Add.gotoAndStop("Options_SP");
				OptionsBtn.Line.gotoAndStop("long");
				OptionsBtn.x = 1485.90 - 70;
	
				QuitBtn.Text.gotoAndStop("Quit_SP");
				QuitBtn.Add.gotoAndStop("Quit_SP");
				QuitBtn.Line.gotoAndStop("short");
				
				MainMenuBtn.TextFX.gotoAndStop("sp");
				MainMenuBtn.Pressed.gotoAndStop("sp");
				MainMenuBtn.Over.gotoAndStop("sp");
				MainMenuBtn.Off.gotoAndStop("sp");
			}
		
		}
	}
}
