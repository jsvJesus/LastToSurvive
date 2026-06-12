package warz.events  
{
	import warz.events.Events;
	
	/**
	 * Events dispatched by the Pause that may or may not be specific to a single screen
	 */
	
	public class VaultEvents extends Events
	{
		public static function eventReturnToGame()
		{
			send("eventReturnToGame");
		}

		public static function eventBackpackFromInventory (inventoryID:uint, gridTo:int, amount:int)
		{
			send("eventBackpackFromInventory", inventoryID, gridTo, amount);
		}
		public static function eventBackpackToInventory (gridFrom:int, amount:int)
		{
			send("eventBackpackToInventory", gridFrom, amount);
		}
		public static function eventBackpackGridSwap (gridFrom:int, gridTo:int)
		{
			send("eventBackpackGridSwap", gridFrom, gridTo);
		}		
	}
}