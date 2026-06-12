package warz.dataObjects  {
	public class Item  {

		public	var	itemID:uint;
		public	var	catID:int;
		public	var	Name:String;
		public	var	desc:String; 
		public	var	Icon:String; 
		public	var	stats:Array = new Array();
		public	var	isStackable:Boolean;
		public	var	weight:Number;
		public	var	maxSlots:int = -1;				// backpack only
		public	var	maxWeight:Number;				// backpack only
		
		public	function Item (itemID:uint, catID:int, name:String, desc:String, 
							   icon:String, stats:Array, isStackable:Boolean, weight:Number, maxSlots:int, maxWeight:Number)
		{
			this.itemID = itemID;
			this.catID = catID;
			this.Name = name;
			this.desc = desc;
			this.Icon = icon;
			this.stats = stats;
			this.isStackable = isStackable;
			this.maxSlots = maxSlots;
			this.maxWeight = maxWeight;
			this.weight = weight;
		}
	}
}