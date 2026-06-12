package warz.utils
{
	import flash.display.Bitmap;
	import flash.display.BitmapData;
	import flash.geom.Matrix;

	public class Layout
	{
		public static function stretch(clip:Bitmap, preferredWidth:Number, preferredHeight:Number, viewWidth:Number, viewHeight:Number, scaleMode:String):Bitmap
		{
			var newW	:Number = preferredWidth;
			var newH	:Number = preferredHeight;
			var xscale	:Number = viewWidth / preferredWidth;
			var yscale	:Number = viewHeight / preferredHeight;
			
			switch (scaleMode.toLowerCase()) {
				case "none":
					break;
				case "exact":
					newW = preferredWidth;
					newH = preferredHeight;
					break;
				case "fill":
					if (xscale > yscale) {
						newW *= xscale;
						newH *= xscale;
					} else {
						newW *= yscale;
						newH *= yscale;
					}
					break;
				case "uniform":
				default:
					if (xscale < yscale) {
						newW = viewWidth / yscale;
						newH = viewHeight / yscale;
					} else {
						newW = viewWidth / xscale;
						newH = viewHeight / xscale;
					}
					break;
			}
			
			clip.width = Math.ceil(newW);
			clip.height = Math.ceil(newH);
			clip.x = -(clip.width / 2);
			clip.y = -(clip.height / 2);
			return clip;
		}		
	}	
}
