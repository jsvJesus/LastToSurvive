package warz.repair {
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import warz.events.RepairEvents;
	import warz.events.SoundEvents;
	
	public class msgBox extends MovieClip {
		
		public	var	Msgbox:MovieClip;
		
		public	var	callback:*;
		
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
		
		public function showInfoInputMsg(msg:String, title:String = "", callback:* = null)
		{
			Msgbox.InputBox.visible = true;
			Msgbox.InputBox.InputText.text = "";
			Msgbox.InputBox.InputText.displayAsPassword = false;
			Msgbox.Title.text = title;
			visible = true;
			this.callback = callback;
			
			if (title == "")
			{
				Msgbox.Backlight.visible = false;
			}
			
			Msgbox.Text.text = msg;
			Msgbox.OKBtn.visible = false;
			Msgbox.Progress.visible = false;
			
			Msgbox.OKBtn2.visible = true;
			Msgbox.OKBtn2.Text.Text.text = "$FR_OK";
			Msgbox.CancelBtn.visible = true;
			Msgbox.CancelBtn.Text.Text.text = "$FR_Cancel";
			
			var	me = this;
			
			Msgbox.OKBtn2.Btn.addEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			Msgbox.OKBtn2.Btn.addEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			Msgbox.OKBtn2.Btn.addEventListener(MouseEvent.CLICK, MouseOk3Click);

			Msgbox.CancelBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			Msgbox.CancelBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			Msgbox.CancelBtn.Btn.addEventListener(MouseEvent.CLICK, MouseCancelClick);
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
		
		public	function MouseOk3Click (evt:MouseEvent):void
		{			
			if(Msgbox.InputBox.InputText.length == 0)
				return;
		
			SoundEvents.eventSoundPlay("menu_click");
			
			Msgbox.OKBtn2.Btn.removeEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			Msgbox.OKBtn2.Btn.removeEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			Msgbox.OKBtn2.Btn.removeEventListener(MouseEvent.CLICK, MouseOk3Click);
			
			Msgbox.CancelBtn.Btn.removeEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			Msgbox.CancelBtn.Btn.removeEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			Msgbox.CancelBtn.Btn.removeEventListener(MouseEvent.CLICK, MouseCancelClick);

			visible = false; 
			if (callback)
				callback (true, Msgbox.InputBox.InputText.text);
		}

		public	function MouseCancelClick (evt:MouseEvent):void
		{
			SoundEvents.eventSoundPlay("menu_click");
			
			Msgbox.OKBtn2.Btn.removeEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			Msgbox.OKBtn2.Btn.removeEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			Msgbox.OKBtn2.Btn.removeEventListener(MouseEvent.CLICK, MouseOk3Click);

			Msgbox.CancelBtn.Btn.removeEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			Msgbox.CancelBtn.Btn.removeEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			Msgbox.CancelBtn.Btn.removeEventListener(MouseEvent.CLICK, MouseCancelClick);

			visible = false; 
			if (callback)
				callback (false);
		}
		
	}
}