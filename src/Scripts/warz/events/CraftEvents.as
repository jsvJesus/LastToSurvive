package warz.events  
{
	import warz.events.Events;
	
	/**
	 * Events dispatched by the Pause that may or may not be specific to a single screen
	 */
	
	public class CraftEvents extends Events
	{
		public static function eventReturnToGame()
		{
			send("eventReturnToGame");
		}

		public static function eventCraftItem(recipeID:uint)
		{
			send("eventCraftItem", recipeID);
		}
	}
}