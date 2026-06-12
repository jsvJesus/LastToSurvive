package warz.frontend {
	import flash.display.MovieClip;
	import warz.dataObjects.*;
	import flash.events.*;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;

	public class PlayGameQuickScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;

		public var QuickJoin:MovieClip;
		
		public	function PlayGameQuickScreen ()
		{
			visible = false;

			QuickJoin.Btn.addEventListener(MouseEvent.MOUSE_OVER, function() {});
			QuickJoin.Btn.addEventListener(MouseEvent.MOUSE_OUT, function() {});
			QuickJoin.Btn.addEventListener(MouseEvent.CLICK, function() {});


			/*QuickJoin.BtnCancel.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			QuickJoin.BtnCancel.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			QuickJoin.BtnCancel.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			QuickJoin.BtnCancel.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);

			QuickJoin.BtnCancel.ActionFunction = function()
			{
				FrontEndEvents.eventCancelQuickGameSearch();
				warz.frontend.Frontend.api.Main.showScreen("PlayGame");
			}*/
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
			SoundEvents.eventSoundPlay("menu_click");
			evt.currentTarget.parent.gotoAndPlay("pressed");
		}
		private function BtnPressUpFn(evt:Event)
		{
			evt.currentTarget.parent.ActionFunction();
		}
		
		public function Activate()
		{
			this.visible = true;
			FrontEndEvents.eventPlayGame ();
		}
		
		public function Deactivate()
		{
			this.visible = false;
		}
	}
}