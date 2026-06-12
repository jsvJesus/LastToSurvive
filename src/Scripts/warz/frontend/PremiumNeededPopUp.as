package warz.frontend {
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;
	
	public class PremiumNeededPopUp extends MovieClip {
		
		public	var	BtnCancel:MovieClip;
		public	var	BtnBuy:MovieClip;
		
		public	function PremiumNeededPopUp ()
		{
			visible = false;
			
			Title.text = "$FR_PremiumAccountRequired";
			Text.text = "$FR_PremiumAccountBenefits";
			
			BtnBuy.Text.Text.text = "$FR_Buy";
			BtnCancel.Text.Text.text = "$FR_Cancel";
			
			BtnBuy.Btn.addEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			BtnBuy.Btn.addEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			BtnBuy.Btn.addEventListener(MouseEvent.CLICK, MouseOkClick);

			BtnCancel.Btn.addEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			BtnCancel.Btn.addEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			BtnCancel.Btn.addEventListener(MouseEvent.CLICK, MouseCancelClick);
		}
		
		public function showPopUp()
		{
			Price.text = String(warz.frontend.Frontend.api.PremiumAccount_Price);
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
			FrontEndEvents.eventBuyPremiumAccount();			
		}

		public	function MouseCancelClick (evt:MouseEvent):void
		{
			SoundEvents.eventSoundPlay("menu_click");
			visible = false; 
		}
	}
}