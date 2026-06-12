package warz.pickupUi {
	import flash.display.MovieClip;

	public class main extends MovieClip {
		
		public	var api:warz.pickupUi.PickupUI = null;
		
		public	var	Plate:MovieClip;
		public	var	E_Block:MovieClip;
		public  var CarUI:MovieClip;

		public function Main() {
			
		}
		
		public	function init ():void
		{
			CarUI.visible = false;
		}

		// durability: [0-5], speed,armor,weight:[0,100], gas: [0,100]
		public function setCarInfo(durability:int, speed:int, armor:int, weight:int, gas:int)
		{
			CarUI.ProgressBarCond1.visible = (durability >= 1);
			CarUI.ProgressBarCond2.visible = (durability >= 2);
			CarUI.ProgressBarCond3.visible = (durability >= 3);
			CarUI.ProgressBarCond4.visible = (durability >= 4);
			CarUI.ProgressBarCond5.visible = (durability >= 5);
			
			CarUI.ProgressBarsLow1.width = 190*(speed/100);
			CarUI.ProgressBarsLow2.width = 190*(armor/100);
			CarUI.ProgressBarsLow3.width = 190*(weight/100);
			
			CarUI.Gas.Text.text = gas+"%";
			if(gas<=0) gas = 1;
			CarUI.Gas.Progress.gotoAndStop(gas);
		}
		
		public function setCarInfoVisibility(vis:Boolean)
		{
			CarUI.visible = vis;
		}

		public	function showPlateText (text:String):void
		{
			setCarInfoVisibility(false);
			Plate.Text.text = text;
			Plate.TextShad.text = text;
		}
	
		public	function showPlateTitle (text:String):void
		{
			setCarInfoVisibility(false);
			Plate.Name.text = text;
		}

		public	function setEBlockLetter (letter:String):void
		{
			E_Block.E.E.text = letter;
			E_Block.E2.E.text = letter;
		}
		
		public	function showEBlock (state:Boolean):void
		{
			E_Block.visible = state;
		}
		
		public	function setEBlockToRegular ():void
		{
			if (E_Block.currentLabel != "regular")
				E_Block.gotoAndPlay ("regular");
		}

		public	function setEBlockToHold ():void
		{
			if (E_Block.currentLabel != "hold")
				E_Block.gotoAndPlay ("hold");
		}
		
		public	function setEBlockProgress (value:int):void
		{
			E_Block.Progress.gotoAndStop (value);
		}
	}
}
