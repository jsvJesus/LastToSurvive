package warz.dataObjects  {
	public class StoreItem {

		public	var	itemID:uint;
		public	var	price:int;
		public	var	priceGD:int;
		public	var	newItem:Boolean;
		public	var	quantity:int;

		public	function StoreItem (itemID:uint, price:int, priceGD:int, quantity:int, newItem:Boolean)
		{
			this.itemID = itemID;
			this.price = price;
			this.priceGD = priceGD;
			this.newItem = newItem;
			this.quantity = quantity;
		}
	}
}