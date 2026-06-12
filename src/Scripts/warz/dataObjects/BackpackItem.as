package warz.dataObjects  {
	public class BackpackItem {

		public	var	inventoryID:uint;
		public	var	slotID:int;
		public	var	itemID:uint;
		public	var	quantity:int;
		public	var	var1:int;
		public	var	var2:int;
		public	var	isConsumable:Boolean;
		public	var	Description:String;

		public	function BackpackItem (slotID:int, inventoryID:uint, itemID:uint, quantity:int, var1:int, var2:int, isConsumable:Boolean, description:String = "")
		{
			this.inventoryID = inventoryID;
			this.slotID = slotID;
			this.itemID = itemID;
			this.quantity = quantity;
			this.var1 = var1;
			this.var2 = var2;
			this.isConsumable = isConsumable;
			this.Description = description;
		}
	}
}