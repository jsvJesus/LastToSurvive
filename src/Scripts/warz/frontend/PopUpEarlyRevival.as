package warz.frontend {
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;
	
	public class PopUpEarlyRevival extends MovieClip {
		
		public	var	CancelBtn:MovieClip;
		public	var	ApplyBtn:MovieClip;
		public	var	Value:MovieClip;
		
		public	function PopUpEarlyRevival ()
		{
			visible = false;
			
			ApplyBtn.Text.Text.text = "$FR_OK";
			CancelBtn.Text.Text.text = "$FR_Cancel";
			
			ApplyBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			ApplyBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			ApplyBtn.Btn.addEventListener(MouseEvent.CLICK, MouseOkClick);

			CancelBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			CancelBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			CancelBtn.Btn.addEventListener(MouseEvent.CLICK, MouseCancelClick);
		}
		
		public function showPopUp()
		{
			Value.Value.text = warz.frontend.Frontend.api.EarlyRevival_Price;
			visible = true;
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
			FrontEndEvents.eventReviveCharMoney();			
		}

		public	function MouseCancelClick (evt:MouseEvent):void
		{
			SoundEvents.eventSoundPlay("menu_click");
			visible = false; 
		}
	}
}