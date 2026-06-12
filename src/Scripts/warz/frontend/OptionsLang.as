package warz.frontend {
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import flash.events.Event;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;
	
	public class OptionsLang extends MovieClip {
		public var isActiveScreen:Boolean = false;

		public	var	OptionsLangClip:MovieClip;
	
		public var api:warz.frontend.Frontend=null;

		public	function OptionsLang ()
		{
			OptionsLangClip.General_Options.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsLangClip.General_Options.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsLangClip.General_Options.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsLangClip.Controls.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsLangClip.Controls.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsLangClip.Controls.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsLangClip.Language.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsLangClip.Language.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsLangClip.Language.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);

			OptionsLangClip.Voip.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsLangClip.Voip.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsLangClip.Voip.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);

			OptionsLangClip.BtnApply.visible = false;
			OptionsLangClip.BtnReset.visible = false;
			
			OptionsLangClip.Lang1.Name.Name.text = "$FR_optLangEnglish";
			OptionsLangClip.Lang1.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsLangClip.Lang1.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsLangClip.Lang1.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsLangClip.Lang2.visible = true;
			OptionsLangClip.Lang2.Name.Name.text = "$FR_optLangFrench";
			OptionsLangClip.Lang2.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsLangClip.Lang2.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsLangClip.Lang2.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsLangClip.Lang3.visible = true;
			OptionsLangClip.Lang3.Name.Name.text = "$FR_optLangGerman";
			OptionsLangClip.Lang3.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsLangClip.Lang3.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsLangClip.Lang3.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsLangClip.Lang4.visible = true;
			OptionsLangClip.Lang4.Name.Name.text = "$FR_optLangItalian";
			OptionsLangClip.Lang4.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsLangClip.Lang4.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsLangClip.Lang4.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsLangClip.Lang5.visible = true;
			OptionsLangClip.Lang5.Name.Name.text = "$FR_optLangSpanish";
			OptionsLangClip.Lang5.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsLangClip.Lang5.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsLangClip.Lang5.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsLangClip.Lang6.visible = true;
			OptionsLangClip.Lang6.Name.Name.text = "$FR_optLangRussian";
			OptionsLangClip.Lang6.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsLangClip.Lang6.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsLangClip.Lang6.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
		}
		
		public function Activate()
		{
			api = warz.frontend.Frontend.api;

			this.gotoAndPlay("start");

			OptionsLangClip.Language.State = "active";
			
			if (OptionsLangClip.Language.currentLable != "pressed")
				OptionsLangClip.Language.gotoAndPlay("pressed");

			api.setButtonText (OptionsLangClip.General_Options.Text, "$FR_optGeneral");
			api.setButtonText (OptionsLangClip.Controls.Text, "$FR_optControls");
			api.setButtonText (OptionsLangClip.Language.Text, "$FR_optLanguage");
			api.setButtonText (OptionsLangClip.Voip.Text, "$FR_optVoip");
		}
		 
		public function Deactivate()
		{
			this.gotoAndPlay("end");
		}

		private function SlotRollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				if (evt.currentTarget.parent.currentLabel != "over")
					evt.currentTarget.parent.gotoAndPlay("over");
			}
		}
		
		private function SlotRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				if (evt.currentTarget.parent.currentLabel != "out")
					evt.currentTarget.parent.gotoAndPlay("out");
			}
		}
		
		private function SlotPressFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
				
				var	p:MovieClip = evt.currentTarget.parent;
				
				while (!(p as OptionsLang))
				{
					p = p.parent as MovieClip;
				}
				
				SoundEvents.eventSoundPlay("menu_click");
				
				(p as OptionsLang).ActionFunction(evt.currentTarget.parent.name);
			}
		}
		
		public function ActionFunction (button:String)
		{
			if (button == "General_Options")
			{
				api.showOptionsGeneral();
				
				OptionsLangClip.General_Options.State = "off";
				OptionsLangClip.General_Options.gotoAndPlay("out");
			}
			else if (button == "Controls")
			{
				api.showOptionsControls();
				
				OptionsLangClip.Controls.State = "off";
				OptionsLangClip.Controls.gotoAndPlay("out");
			}
			else if (button == "Language")
			{
//				api.showOptionsLanguage();
				OptionsLangClip.Language.State = "off";
				OptionsLangClip.Language.gotoAndPlay("out");
			}
			else if (button == "Voip")
			{
				api.showOptionsVoip();
				OptionsLangClip.Voip.State = "off";
				OptionsLangClip.Voip.gotoAndPlay("out");
			}
			else if (button.indexOf("Lang") != -1)
			{
				var lang:int = int (button.slice(4)) - 1;
				var	languages:Array = new Array ("english", "french", "german", "italian", "spanish", "russian");
				
				FrontEndEvents.eventOptionsLanguageSelection (languages[lang]);

				OptionsLangClip[button].State = "off";
				OptionsLangClip[button].gotoAndPlay("out");
			}
		}
	}
}
