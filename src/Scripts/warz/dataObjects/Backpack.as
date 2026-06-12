package warz.dataObjects  {
	import warz.dataObjects.*;

	public class Backpack  {
		public	var	BackpackDB:Array = null;
		public	var	backpackSize:int = 0;
		
		public	function Backpack (size:int):void
		{
			BackpackDB = new Array ();
			this.backpackSize = size;
		}
		
		public	function addItem (slotID:int, item:BackpackItem):void
		{
			if (slotID >= backpackSize)
				return;
				
			BackpackDB[slotID] = item;
		}
		
		public	function getBackpackItem (slotID:int):BackpackItem
		{
			if (slotID < backpackSize)
				return BackpackDB[slotID];
				
			return null;
		}
		
		public	function findBackpackItem (itemID:uint):BackpackItem
		{
			for (var a:int = 0; a < backpackSize; a++)
			{
				var	item:BackpackItem = BackpackDB[a];
				
				if (item && item.itemID == itemID)
					return item;
			}
			
			return null;
		}
		
		public	function clearBackpack ():void
		{
			for (var a:int = 0; a < backpackSize; a++)
			{
				BackpackDB[a] = null;
			}			
		}
		
		public function removeBackpackItem (slotID:int):void
		{
			if (slotID < backpackSize)
			{
				BackpackDB[slotID] = null;
			}
		}		 
		
		public	function swapBackpackSlots(slot1:int, slot2:int):void
		{
			var	temp1:BackpackItem = BackpackDB[slot1];
			var	temp2:BackpackItem = BackpackDB[slot2];
			
			BackpackDB[slot1] = temp2;
			BackpackDB[slot2] = temp1;			
			
			if (temp1)
				temp1.slotID = slot2;
			if (temp2)
				temp2.slotID = slot1;
		}
	}
}