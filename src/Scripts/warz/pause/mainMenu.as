package warz.pause {
	import flash.display.MovieClip;
	import flash.events.Event;
	import flash.events.MouseEvent;
	import warz.events.PauseEvents;
	import warz.events.SoundEvents;

	public class mainMenu extends MovieClip {
		
		public var api:warz.pause.pause=null;
		
		public	var	MainMenu:MovieClip;
		
		public function mainMenu() {
		}
		
		public function Activate()
		{
			api = warz.pause.pause.api;

			this.mouseEnabled = true;
			this.mouseChildren = true;
			this.visible = true;
			
			MainMenu.MainMenuBtn1.Btn.addEventListener(MouseEvent.MOUSE_OVER, RollOverBtnFn);
			MainMenu.MainMenuBtn1.Btn.addEventListener(MouseEvent.MOUSE_OUT, RollOutBtnFn);
			MainMenu.MainMenuBtn1.Btn.addEventListener(MouseEvent.CLICK, PressBtnFn);
			MainMenu.MainMenuBtn2.Btn.addEventListener(MouseEvent.MOUSE_OVER, RollOverBtnFn);
			MainMenu.MainMenuBtn2.Btn.addEventListener(MouseEvent.MOUSE_OUT, RollOutBtnFn);
			MainMenu.MainMenuBtn2.Btn.addEventListener(MouseEvent.CLICK, PressBtnFn);
			MainMenu.MainMenuBtn3.Btn.addEventListener(MouseEvent.MOUSE_OVER, RollOverBtnFn);
			MainMenu.MainMenuBtn3.Btn.addEventListener(MouseEvent.MOUSE_OUT, RollOutBtnFn);
			MainMenu.MainMenuBtn3.Btn.addEventListener(MouseEvent.CLICK, PressBtnFn);
			MainMenu.MainMenuBtn4.Btn.addEventListener(MouseEvent.MOUSE_OVER, RollOverBtnFn);
			MainMenu.MainMenuBtn4.Btn.addEventListener(MouseEvent.MOUSE_OUT, RollOutBtnFn);
			MainMenu.MainMenuBtn4.Btn.addEventListener(MouseEvent.CLICK, PressBtnFn);
			MainMenu.MainMenuBtn5.Btn.addEventListener(MouseEvent.MOUSE_OVER, RollOverBtnFn);
			MainMenu.MainMenuBtn5.Btn.addEventListener(MouseEvent.MOUSE_OUT, RollOutBtnFn);
			MainMenu.MainMenuBtn5.Btn.addEventListener(MouseEvent.CLICK, PressBtnFn);
			MainMenu.MainMenuBtn6.Btn.addEventListener(MouseEvent.MOUSE_OVER, RollOverBtnFn);
			MainMenu.MainMenuBtn6.Btn.addEventListener(MouseEvent.MOUSE_OUT, RollOutBtnFn);
			MainMenu.MainMenuBtn6.Btn.addEventListener(MouseEvent.CLICK, PressBtnFn);
			
			MainMenu.MainMenuBtn1.Text.Text.text = "$FR_Inventory"
			MainMenu.MainMenuBtn1.TextShad.Text.text = "$FR_Inventory"
			MainMenu.MainMenuBtn2.Text.Text.text = "$FR_MapType"
			MainMenu.MainMenuBtn2.TextShad.Text.text = "$FR_MapType"
			MainMenu.MainMenuBtn3.Text.Text.text = "$FR_Missions"
			MainMenu.MainMenuBtn3.TextShad.Text.text = "$FR_Missions"
			MainMenu.MainMenuBtn4.Text.Text.text = "$FR_Options"
			MainMenu.MainMenuBtn4.TextShad.Text.text = "$FR_Options"
			MainMenu.MainMenuBtn5.Text.Text.text = "$FR_PAUSE_QUIT"
			MainMenu.MainMenuBtn5.TextShad.Text.text = "$FR_PAUSE_QUIT"
			MainMenu.MainMenuBtn6.Text.Text.text = "$FR_PAUSE_BACKTOGAME"
			MainMenu.MainMenuBtn6.TextShad.Text.text = "$FR_PAUSE_BACKTOGAME"
		}
		
		public function Deactivate()
		{
		}
		
		public	function RollOverBtnFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				evt.currentTarget.parent.gotoAndPlay("over");
			}
		}
		
		public	function RollOutBtnFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				evt.currentTarget.parent.gotoAndPlay("out");
			}
		}
		
		public	function PressBtnFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				for(var i=0; i<6; ++i)
				{
					var	name = "MainMenuBtn" + String (i + 1);
					
					if(MainMenu[name].State == "active")
					{
						if (evt.currentTarget.parent.name != "MainMenuBtn5" &&
							evt.currentTarget.parent.name != "MainMenuBtn6")
						{
							MainMenu[name].State = "off";
							MainMenu[name].gotoAndPlay("out");
							
							switch (i)
							{
								case	0:
									(parent as main).showInventory (false);
									break;
								case	1:
									(parent as main).showMap (false);
									break;
								case	2:
									(parent as main).showMissions(false);
									break;
								case	3:
									(parent as main).showOptions(false);
									break;
							}
						}
					}
				}
				SoundEvents.eventSoundPlay("menu_click");

				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
				ActionFunction(evt.currentTarget.parent.name);
			}
		}
		
		public	function ActionFunction (button:String)
		{
			if (button == "MainMenuBtn1")
			{
				(parent as main).showInventory (true);
			}
			else if (button == "MainMenuBtn2")
			{
				(parent as main).showMap (true);
			}
			else if (button == "MainMenuBtn3")
			{
				(parent as main).showMissions (true);
			}
			else if (button == "MainMenuBtn4")
			{
				(parent as main).showOptions (true);
			}
			else if (button == "MainMenuBtn5")
			{
				if (MainMenu.MainMenuBtn5.currentLabel != "out")
					MainMenu.MainMenuBtn5.gotoAndPlay("out");
					
				MainMenu.MainMenuBtn5.State = "off";
				api.showInfoOkCancelMsg ("$FR_PAUSE_QUIT_SURE", "$FR_PAUSE_QUIT_GAME", quitGameCallback);
			}
			else if (button == "MainMenuBtn6")
			{
				if (MainMenu.MainMenuBtn6.currentLabel != "out")
					MainMenu.MainMenuBtn6.gotoAndPlay("out");
					
				MainMenu.MainMenuBtn6.State = "off";
				PauseEvents.eventBackToGame();
			}
		}
		
		public	function quitGameCallback (state:Boolean):void
		{
			if (state)
				PauseEvents.eventQuitGame();
		}
	}
}
