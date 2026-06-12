package warz.events  
{
	import warz.events.Events;
	
	/**
	 * Events dispatched by the FrontEnd that may or may not be specific to a single screen
	 */
	
	public class MarketEvents extends Events
	{
		public static function eventReturnToGame()
		{
			send("eventReturnToGame");
		}
		
		public static function eventBuyItem (itemID:uint, price:int, priceGD:int)
		{
			send("eventBuyItem", itemID, price, priceGD);
		}
	}
}