package warz.events  
{
	import warz.events.Events;
	
	/**
	 * Events dispatched by the Pause that may or may not be specific to a single screen
	 */
	
	public class RepairEvents extends Events
	{
		public static function eventReturnToGame()
		{
			send("eventReturnToGame");
		}

		public static function eventRepairAll()
		{
			send("eventRepairAll");
		}
		
		public static function eventRepairItemInSlot(slotID:uint)
		{
			send("eventRepairItemInSlot", slotID);
		}
		
	}
}