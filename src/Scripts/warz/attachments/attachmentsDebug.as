package warz.attachments  {
	import flash.utils.Timer;

	public class attachmentsDebug {

		static public function initDebug(api:warz.attachments.attachments) {
			api.addAttachment (0, 100, 100, 1);
			api.addSlot (0, 0, "Test", 9);
			api.addSlot (0, 1, "Test 2", 2);
			api.setSlotActive(0, 0);

			api.addAttachment (1, 200, 350, 2);
			api.addSlot (1, 0, "Test", 65465);
			api.addSlot (1, 1, "Test 2", 6534637);
			api.addSlot (1, 2, "Give me Life", 54353);
			api.setSlotActive(1, 1);
		}
	}
}