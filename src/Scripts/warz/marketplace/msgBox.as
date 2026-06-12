package warz.marketplace {
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import warz.events.SoundEvents;
	
	public class msgBox extends MovieClip {
		
		public	var	Msgbox:MovieClip;
		
		public	function msgBox ()
		{
			visible = false;
		}
		
		public function showInfoMsg(msg:String, OKButton:Boolean, title:String = "")
		{
			Msgbox.InputBox.visible = false;
			visible = true;
			Msgbox.Title.text = title;
			
			if (title == "")
			{
				Msgbox.Backlight.visible = false;
			}
			
			Msgbox.Text.text = msg;
			Msgbox.OKBtn.visible = OKButton;
			Msgbox.OKBtn.Text.Text.text = "$FR_OK";
			Msgbox.Progress.visible = !OKButton;
			
			Msgbox.OKBtn2.visible = false;
			Msgbox.CancelBtn.visible = false;
			
			var	me = this;
			
			if (OKButton)
			{
				Msgbox.OKBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, MouseOver);
				Msgbox.OKBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, MouseOut);
				Msgbox.OKBtn.Btn.addEventListener(MouseEvent.CLICK, MouseOkClick);
			}
		}
		
		public	function MouseOver (evt:MouseEvent):void
		{
			if (evt.currentTarget.parent.currentLabel != "over")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		
		public	function MouseOut (evt:MouseEvent):void
		{
			if (evt.currentTarget.parent.currentLabel != "out")
				evt.currentTarget.parent.gotoAndPlay("out");
		}

		public	function MouseOkClick (evt:MouseEvent):void
		{
			visible = false; 
			
			SoundEvents.eventSoundPlay("menu_click");
			
			Msgbox.OKBtn.Btn.removeEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			Msgbox.OKBtn.Btn.removeEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			Msgbox.OKBtn.Btn.removeEventListener(MouseEvent.CLICK, MouseOkClick);
		}
	}
}