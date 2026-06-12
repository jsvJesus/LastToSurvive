package warz.frontend {
	import flash.display.MovieClip;
	import warz.dataObjects.*;
	import flash.events.*;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;

	public class PlayGameScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;

		public var api:warz.frontend.Frontend=null;
		
		public var PlayGame:MovieClip;
		
		private var BtnNames:Array = new Array();
		
		public	function PlayGameScreen ()
		{
			visible = false;
			
			BtnNames.push("BtnMyServers");
			BtnNames.push("BtnQJoin");
			BtnNames.push("BtnPlay");
			BtnNames.push("BtnSBrowser");
			
			PlayGame.BtnQJoin.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			PlayGame.BtnQJoin.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			PlayGame.BtnQJoin.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			PlayGame.BtnQJoin.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			PlayGame.BtnSBrowser.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			PlayGame.BtnSBrowser.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			PlayGame.BtnSBrowser.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			PlayGame.BtnSBrowser.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);

			PlayGame.BtnMyServers.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			PlayGame.BtnMyServers.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			PlayGame.BtnMyServers.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			PlayGame.BtnMyServers.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);


			PlayGame.BtnPlay.gotoAndStop("inactive");
			//PlayGame.BtnMyStrongholds.gotoAndStop("inactive");
			//PlayGame.BtnMyStrongholds.visible = false;

			PlayGame.BtnQJoin.ActionFunction = function()
			{
				api.Main.showScreen("PlayGameQuick");
			}
			PlayGame.BtnSBrowser.ActionFunction = function()
			{
				api.Main.showScreen("ServerBrowse");
			}
			PlayGame.BtnMyServers.ActionFunction = function()
			{
				api.Main.showScreen("MyServers");
			}
		}
		
		private function BtnRollOverFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("over");
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
					if(PlayGame[BtnNames[i]].State == "active")
					{
						PlayGame[BtnNames[i]].State = "off";
						PlayGame[BtnNames[i]].gotoAndPlay("out");
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
			api = warz.frontend.Frontend.api;
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
			PlayGame.Name.text = survivor.Name;
			var	mins:int = (survivor.timeplayed / 60) % 60;
			var	hours:int = (survivor.timeplayed / 3600) % 24;
			var	days:int = (survivor.timeplayed / 86400);
			var	time:String = "";
			
			if(api.m_Language == "russian")
			{
				time += String (days) + "Д ";
				time += String (hours) + "Ч ";
				time += String (mins) + "М ";
			}
			else
			{
				time += String (days) + "D ";
				time += String (hours) + "H ";
				time += String (mins) + "M ";
			}

			PlayGame.Time.text = time;

			this.visible = true;
			FrontEndEvents.eventRequestPlayerRender(3);
		}
		
		public function Deactivate()
		{
			FrontEndEvents.eventRequestPlayerRender(0);
			this.visible = false;
		}
		
		/*
		api = warz.frontend.Frontend.api;

				var	survivor:Survivor = api.Survivors[api.SelectedChar];
				
				if (survivor && survivor.health > 0)
				{
					FrontEndEvents.eventPlayGame ();
				}
				else
				{
					var	text:String = "";
					
					if (!survivor)
						text = "$FR_PLAY_GAME_NO_SURVIVORS";
					else
						text = "$FR_PLAY_GAME_SURVIVOR_DEAD";
					
					api.showInfoMsg (text, true, "$FR_PlayGame");
					MainMenuBtn.State = "off";
					MainMenuBtn.gotoAndPlay("out");
				}
		*/
	}
}