package warz.pickupUi {
	import flash.display.MovieClip;
	import caurina.transitions.Tweener;
	
	public class PickupUI extends MovieClip {
		public var isDebug:Boolean = false;
		
		public var Main:MovieClip = null;

		static public var api:PickupUI = null;

		public function PickupUI(main:MovieClip) 
		{
			api = this;
			Main = main;
			Main.init ();
			
			Tweener.init();
		}
		
		public	function showPlateText (text:String):void
		{
			Main.showPlateText (text);
		}

		public	function showPlateTitle (text:String):void
		{
			Main.showPlateTitle (text);
		}

		public	function setEBlockLetter (letter:String):void
		{
			Main.setEBlockLetter (letter);
		}
		
		public	function setEBlockToRegular ():void
		{
			Main.setEBlockToRegular ();
		}

		public	function setEBlockToHold ():void
		{
			Main.setEBlockToHold ();
		}
		
		/**
		 * setEBlockProgress
		 *
		 * value - 0 - 100: 0 = Empty, 100 = Full
		 **/
		public	function setEBlockProgress (value:int):void
		{
			Main.setEBlockProgress (value);
		}
	}
}