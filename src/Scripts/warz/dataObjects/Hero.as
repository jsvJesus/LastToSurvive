package warz.dataObjects {
	public class Hero {
	
		public	var	itemID:uint;
		public	var	category:int;
		public	var	name:String;
		public	var	desc:String;
		public	var	icon:String; // icon in create survivor screen
		public  var icon2:String; // icon in survivors screen
		public	var icon3:String; // icon in unlock character screen
		public	var	numHeads:int;
		public	var	numBodys:int;
		public	var	numLegs:int;
	
		public	function Hero (itemID:uint, category:int, name:String, desc:String, icon:String, icon2:String, icon3:String, numHeads:int, numBody:int, numLegs:int)
		{
			this.itemID = itemID;
			this.category = category;
			this.name = name;
			this.desc = desc;
			this.icon = icon;
			this.icon2 = icon2;
			this.icon3 = icon3;
			this.numHeads = numHeads;
			this.numBodys = numBody;
			this.numLegs = numLegs;
		}
	}
}