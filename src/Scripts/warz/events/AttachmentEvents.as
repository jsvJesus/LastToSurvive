package warz.events  
{
	import warz.events.Events;
	
	/**
	 * Events dispatched by the Pause that may or may not be specific to a single screen
	 */
	
	public class AttachmentEvents extends Events
	{
		public static function eventSelectAttachment (id:int, slotID:int, attachmentID:int)
		{
			send("eventSelectAttachment", id, slotID, attachmentID);
		}		
	}
}