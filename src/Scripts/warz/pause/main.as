package warz.pause {
	import flash.display.MovieClip;
	
	public class main extends MovieClip {
		
		public	var		Inventory:MovieClip;
		public	var		MsgBox:MovieClip;
		public	var		MainMenu:MovieClip;
		public	var		Map:MovieClip;
		public	var		Options:MovieClip;
		public	var		PopupChangebp:MovieClip;
		public  var		Missions:MovieClip;
		
		public function main() {
			MsgBox.visible = false;
			Inventory.visible = false;
			MainMenu.visible = false;
			Map.visible = false;
			Options.visible = false;
			Missions.visible = false;
			PopupChangebp.visible = false;
		}
		
		public	function showInventory (state:Boolean):void
		{
			(MainMenu as mainMenu).Activate();
			
			if (state)
			{
				(Inventory as inventory).Activate();
				
				MainMenu.MainMenu.MainMenuBtn1.State = "active";
				
				if (MainMenu.MainMenu.MainMenuBtn1.currentLabel != "pressed")
					MainMenu.MainMenu.MainMenuBtn1.gotoAndPlay("pressed");
			}
			else
			{
				(Inventory as inventory).Deactivate();
				
				MainMenu.MainMenu.MainMenuBtn1.State = "off";
				MsgBox.MouseOkClick();//cynthia: 841, simulated visual clicking ok button to dismiss pops out window
				
				if (MainMenu.MainMenu.MainMenuBtn1.currentLabel != "out")
					MainMenu.MainMenu.MainMenuBtn1.gotoAndPlay("out");
			}
		}
		
		public	function showMap (state:Boolean):void
		{
			(MainMenu as mainMenu).Activate();
			
			if (state)
			{
				(Map as map).Activate();

				MainMenu.MainMenu.MainMenuBtn2.State = "active";
				
				if (MainMenu.MainMenu.MainMenuBtn2.currentLabel != "pressed")
					MainMenu.MainMenu.MainMenuBtn2.gotoAndPlay("pressed");
			}
			else
			{
				(Map as map).Deactivate();
				
				MainMenu.MainMenu.MainMenuBtn2.State = "off";
				
				if (MainMenu.MainMenu.MainMenuBtn2.currentLabel != "out")
					MainMenu.MainMenu.MainMenuBtn2.gotoAndPlay("out");
			}
		}
		
		public	function showMissions (state:Boolean):void
		{
			(MainMenu as mainMenu).Activate();
			
			if (state)
			{
				(Missions as missions).Activate();

				MainMenu.MainMenu.MainMenuBtn3.State = "active";
				
				if (MainMenu.MainMenu.MainMenuBtn3.currentLabel != "pressed")
					MainMenu.MainMenu.MainMenuBtn3.gotoAndPlay("pressed");
			}
			else
			{
				(Missions as missions).Deactivate();
				
				MainMenu.MainMenu.MainMenuBtn3.State = "off";
				
				if (MainMenu.MainMenu.MainMenuBtn3.currentLabel != "out")
					MainMenu.MainMenu.MainMenuBtn3.gotoAndPlay("out");
			}
		}
		
		public	function showOptions (state:Boolean):void
		{
			(MainMenu as mainMenu).Activate();
			
			if (state)
			{
				(Options as options).Activate();

				MainMenu.MainMenu.MainMenuBtn4.State = "active";
				
				if (MainMenu.MainMenu.MainMenuBtn4.currentLabel != "pressed")
					MainMenu.MainMenu.MainMenuBtn4.gotoAndPlay("pressed");
			}
			else
			{
				(Options as options).Deactivate();

				MainMenu.MainMenu.MainMenuBtn4.State = "off";
				
				if (MainMenu.MainMenu.MainMenuBtn4.currentLabel != "out")
					MainMenu.MainMenu.MainMenuBtn4.gotoAndPlay("out");
			}
		}
		
		public function showInfoOkCancelMsg(msg:String, title:String = "", callback:* = null)
		{
			MsgBox.showInfoOkCancelMsg (msg, title, callback);
		}
		
		public function showInfoMsg(msg:String, OKButton:Boolean, title:String = "")
		{
			MsgBox.showInfoMsg (msg, OKButton, title);
		}
		public function showInfoMsg2(msg:String, OKButtonText:String, title:String = "")
		{
			MsgBox.showInfoMsg2 (msg, OKButtonText, title);
		}
		
		public	function showChangeBackpack ():void
		{
			PopupChangebp.Activate ();
		}
	}
}
