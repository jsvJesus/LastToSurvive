package warz.frontend {
	import flash.display.MovieClip;
	import warz.dataObjects.*;
	import flash.events.*;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;

	public class PlayChannelsScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;

		public var api:warz.frontend.Frontend=null;
		
		public var ServBrowseChannels:MovieClip;
		
		private var BtnNames:Array = new Array();
		
		public	function PlayChannelsScreen ()
		{
			visible = false;
			
			ServBrowseChannels.DescrText.text = "";
			
			BtnNames.push("Btn1");
			BtnNames.push("Btn2");
			BtnNames.push("Btn3");
			BtnNames.push("Btn4");
			BtnNames.push("Btn5");
			BtnNames.push("Btn6");
			BtnNames.push("Btn7");
			BtnNames.push("Btn8");
			
			ServBrowseChannels.Btn1.Text.Text.text ="$FR_TrialServer";
			ServBrowseChannels.Btn2.Text.Text.text ="$FR_OfficialServers";
			ServBrowseChannels.Btn3.Text.Text.text ="$FR_PrivateServers";
			ServBrowseChannels.Btn4.Text.Text.text ="$FR_PremiumServers";
			ServBrowseChannels.Btn5.Text.Text.text ="$FR_Strongholds";
			ServBrowseChannels.Btn6.Text.Text.text ="$FR_PublicTestEnvironment";
			ServBrowseChannels.Btn7.Text.Text.text ="$FR_MyServers";
			ServBrowseChannels.Btn8.Text.Text.text ="$FR_VeteranServers";
			
			ServBrowseChannels.Btn1.PlatesOn.gotoAndStop(1);
			ServBrowseChannels.Btn1.PlatesOff.gotoAndStop(1);
			ServBrowseChannels.Btn2.PlatesOn.gotoAndStop(2);
			ServBrowseChannels.Btn2.PlatesOff.gotoAndStop(2);
			ServBrowseChannels.Btn3.PlatesOn.gotoAndStop(3);
			ServBrowseChannels.Btn3.PlatesOff.gotoAndStop(3);
			ServBrowseChannels.Btn4.PlatesOn.gotoAndStop(4);
			ServBrowseChannels.Btn4.PlatesOff.gotoAndStop(4);
			ServBrowseChannels.Btn5.PlatesOn.gotoAndStop(5);
			ServBrowseChannels.Btn5.PlatesOff.gotoAndStop(5);
			ServBrowseChannels.Btn6.PlatesOn.gotoAndStop(6);
			ServBrowseChannels.Btn6.PlatesOff.gotoAndStop(6);
			ServBrowseChannels.Btn8.PlatesOn.gotoAndStop(7);
			ServBrowseChannels.Btn8.PlatesOff.gotoAndStop(7);
						
			ServBrowseChannels.QuickJoinBtn1.Text.Text.text = "$FR_QuickJoin";
			ServBrowseChannels.QuickJoinBtn2.Text.Text.text = "$FR_QuickJoin";
			ServBrowseChannels.QuickJoinBtn3.Text.Text.text = "$FR_QuickJoin";
			ServBrowseChannels.QuickJoinBtn4.Text.Text.text = "$FR_QuickJoin";
			ServBrowseChannels.QuickJoinBtn5.Text.Text.text = "$FR_QuickJoin";
			ServBrowseChannels.QuickJoinBtn6.Text.Text.text = "$FR_QuickJoin";
			ServBrowseChannels.QuickJoinBtn8.Text.Text.text = "$FR_QuickJoin";
			
			ServBrowseChannels.Btn1.ActionFunction = function()
			{
				FrontEndEvents.eventSetCurrentBrowseChannel(1, false);
			}
			ServBrowseChannels.Btn2.ActionFunction = function()
			{
				FrontEndEvents.eventSetCurrentBrowseChannel(2, false);
			}
			ServBrowseChannels.Btn3.ActionFunction = function()
			{
				FrontEndEvents.eventSetCurrentBrowseChannel(3, false);
			}
			ServBrowseChannels.Btn4.ActionFunction = function()
			{
				FrontEndEvents.eventSetCurrentBrowseChannel(4, false);
			}
			ServBrowseChannels.Btn5.ActionFunction = function()
			{
				FrontEndEvents.eventSetCurrentBrowseChannel(5, false);
			}
			ServBrowseChannels.Btn6.ActionFunction = function()
			{
				FrontEndEvents.eventSetCurrentBrowseChannel(6, false);
			}
			ServBrowseChannels.Btn7.ActionFunction = function()
			{
				api.Main.showScreen("MyServers");
			}
			ServBrowseChannels.Btn8.ActionFunction = function()
			{
				FrontEndEvents.eventSetCurrentBrowseChannel(7, false);
			}

			ServBrowseChannels.QuickJoinBtn1.ActionFunction = function()
			{
				FrontEndEvents.eventSetCurrentBrowseChannel(1, true);
			}
			ServBrowseChannels.QuickJoinBtn2.ActionFunction = function()
			{
				FrontEndEvents.eventSetCurrentBrowseChannel(2, true);
			}
			ServBrowseChannels.QuickJoinBtn3.ActionFunction = function()
			{
				FrontEndEvents.eventSetCurrentBrowseChannel(3, true);
			}
			ServBrowseChannels.QuickJoinBtn4.ActionFunction = function()
			{
				FrontEndEvents.eventSetCurrentBrowseChannel(4, true);
			}
			ServBrowseChannels.QuickJoinBtn5.ActionFunction = function()
			{
				FrontEndEvents.eventSetCurrentBrowseChannel(5, true);
			}
			ServBrowseChannels.QuickJoinBtn6.ActionFunction = function()
			{
				FrontEndEvents.eventSetCurrentBrowseChannel(6, true);
			}
			ServBrowseChannels.QuickJoinBtn8.ActionFunction = function()
			{
				FrontEndEvents.eventSetCurrentBrowseChannel(7, true);
			}
		}
		
		public function initButtons(btn1:Boolean,btn2:Boolean,btn3:Boolean,btn4:Boolean,btn5:Boolean,btn6:Boolean, btn8:Boolean)
		{
			var tempEnabledBtns:Array = new Array();
			if(btn1)
				tempEnabledBtns.push("Btn1");
			if(btn2)
				tempEnabledBtns.push("Btn2");
			if(btn3)
				tempEnabledBtns.push("Btn3");
			if(btn4)
				tempEnabledBtns.push("Btn4");
			if(btn5)
				tempEnabledBtns.push("Btn5");
			if(btn6)
				tempEnabledBtns.push("Btn6");
			if(btn8)
				tempEnabledBtns.push("Btn8");
			
			tempEnabledBtns.push("Btn7");
			for(var i=0; i<tempEnabledBtns.length; ++i)
			{
				ServBrowseChannels[tempEnabledBtns[i]].Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
				ServBrowseChannels[tempEnabledBtns[i]].Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
				ServBrowseChannels[tempEnabledBtns[i]].Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
				ServBrowseChannels[tempEnabledBtns[i]].Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			}
			var tempEnabledBtns2:Array = new Array();
			if(btn1)
				tempEnabledBtns2.push("QuickJoinBtn1");
			if(btn2)
				tempEnabledBtns2.push("QuickJoinBtn2");
			if(btn3)
				tempEnabledBtns2.push("QuickJoinBtn3");
			if(btn4)
				tempEnabledBtns2.push("QuickJoinBtn4");
			if(btn5)
				tempEnabledBtns2.push("QuickJoinBtn5");
			if(btn6)
				tempEnabledBtns2.push("QuickJoinBtn6");
			if(btn8)
				tempEnabledBtns2.push("QuickJoinBtn8");
			for(i=0; i<tempEnabledBtns2.length; ++i)
			{
				ServBrowseChannels[tempEnabledBtns2[i]].Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
				ServBrowseChannels[tempEnabledBtns2[i]].Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
				ServBrowseChannels[tempEnabledBtns2[i]].Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
				ServBrowseChannels[tempEnabledBtns2[i]].Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			}			

			if(!btn1)
				ServBrowseChannels.Btn1.gotoAndStop("locked");
			if(!btn2)
				ServBrowseChannels.Btn2.gotoAndStop("locked");
			if(!btn3)
				ServBrowseChannels.Btn3.gotoAndStop("locked");
			if(!btn4)
				ServBrowseChannels.Btn4.gotoAndStop("locked");
			if(!btn5)
				ServBrowseChannels.Btn5.gotoAndStop("locked");
			if(!btn6)
				ServBrowseChannels.Btn6.gotoAndStop("locked");
			if(!btn8)
				ServBrowseChannels.Btn8.gotoAndStop("locked");

			if(!btn1)
				ServBrowseChannels.QuickJoinBtn1.gotoAndStop("locked");
			if(!btn2)
				ServBrowseChannels.QuickJoinBtn2.gotoAndStop("locked");
			if(!btn3)
				ServBrowseChannels.QuickJoinBtn3.gotoAndStop("locked");
			if(!btn4)
				ServBrowseChannels.QuickJoinBtn4.gotoAndStop("locked");
			if(!btn5)
				ServBrowseChannels.QuickJoinBtn5.gotoAndStop("locked");
			if(!btn6)
				ServBrowseChannels.QuickJoinBtn6.gotoAndStop("locked");
			if(!btn8)
				ServBrowseChannels.QuickJoinBtn8.gotoAndStop("locked");
		}
		
		private function BtnRollOverFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("over");
			if(evt.currentTarget.parent.name=="Btn1")
				ServBrowseChannels.DescrText.text = "$FR_TrialServerDesc";
			else if(evt.currentTarget.parent.name=="Btn2")
				ServBrowseChannels.DescrText.text = "$FR_OfficialServersDesc";
			else if(evt.currentTarget.parent.name=="Btn3")
				ServBrowseChannels.DescrText.text = "$FR_PrivateServersDesc";
			else if(evt.currentTarget.parent.name=="Btn4")
				ServBrowseChannels.DescrText.text = "$FR_PremiumServersDesc";
			else if(evt.currentTarget.parent.name=="Btn5")
				ServBrowseChannels.DescrText.text = "$FR_StrongholdsDesc";
			else if(evt.currentTarget.parent.name=="Btn6")
				ServBrowseChannels.DescrText.text = "$FR_PublicTestEnvironmentDesc";
			else if(evt.currentTarget.parent.name=="Btn7")
				ServBrowseChannels.DescrText.text = "$FR_MyServersDesc";
			else if(evt.currentTarget.parent.name=="Btn8")
				ServBrowseChannels.DescrText.text = "$FR_VeteranServersDesc";
		}
		private function BtnRollOutFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("out");
		}
		private function BtnPressFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				for(var i=0; i<BtnNames.length; ++i)
				{
					if(ServBrowseChannels[BtnNames[i]].State == "active")
					{
						ServBrowseChannels[BtnNames[i]].State = "off";
						ServBrowseChannels[BtnNames[i]].gotoAndPlay("out");
					}
				}
				
				SoundEvents.eventSoundPlay("menu_click");
				
				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
			}
		}
		private function BtnPressUpFn(evt:Event)
		{
			if(evt.currentTarget.parent.State == "active")
			{
				evt.currentTarget.parent.ActionFunction();
				evt.currentTarget.parent.State = "off";
			}
		}
		
		public function Activate()
		{
			ServBrowseChannels.DescrText.text = "";
			
			api = warz.frontend.Frontend.api;
			
			if(api.isDebug)
			{
				initButtons(false, true, true, false, false, false, false);
			}
			
			var	survivor:Survivor = api.Survivors[api.SelectedChar];
			if (!(survivor && survivor.alive > 0))
			{
				var	text:String = "";
				
				if (!survivor)
					text = "$FR_PLAY_GAME_NO_SURVIVORS";
				else
					text = "$FR_PLAY_GAME_SURVIVOR_DEAD";
				
				api.showInfoMsg (text, true, "$FR_PlayGame");
				api.Main.MainMenu.MainMenuBtn.State = "off";
				api.Main.MainMenu.MainMenuBtn.gotoAndPlay("out");

				//api.Main.showPrevScreen();
				api.Main.MainMenu.SurvivorsBtn.Btn.dispatchEvent(new Event(MouseEvent.CLICK));
				return;
			}
			
			this.visible = true;
		}
		
		public function Deactivate()
		{
			this.visible = false;
		}
	}
}