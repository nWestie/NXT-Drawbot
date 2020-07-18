/**
  Adapted from HyperCube CAM post processor for Fusion360(Based on 'Generic Grbl' (grbl.cps) and 'Dumper' (dump.cps)
  Compatible with Marlin 1.1.4+
  For Use with my robots :)
*/
description = "NXTGcode for Fusion360";
vendor = "Marlin";
vendorUrl = "https://github.com/MarlinFirmware/Marlin";

extension = "txt";
setCodePage("ascii");

capabilities = CAPABILITY_INTERMEDIATE;
tolerance = spatial(0.5, MM);

minimumChordLength = spatial(0.5, MM);
minimumCircularRadius = spatial(0.5, MM);
maximumCircularRadius = spatial(1000, MM);
minimumCircularSweep = toRad(0.01);
maximumCircularSweep = toRad(180);
allowHelicalMoves = true;
allowedCircularPlanes = undefined; // allow any circular motion

// user-defined properties
properties = {
  startHomeX: false,
  startHomeY: false,
  startHomeZ: false,
  startPositionZ: "2",
  finishHomeX: false,
  finishPositionY: "",
  finishPositionZ: "",
  finishBeep: false,
  rapidTravelXY: 3600,
  rapidTravelZ: 3600,
  laserEtch: "M3",
  laserVaperize: "M3",
  laserThrough: "M3",
  laserOFF: "M5"
};

var xyzFormat = createFormat({decimals:3});
var feedFormat = createFormat({decimals:0});

var xOutput = createVariable({prefix:"X"}, xyzFormat);
var yOutput = createVariable({prefix:"Y"}, xyzFormat);
var zOutput = createVariable({prefix:"Z"}, xyzFormat);
var feedOutput = createVariable({prefix:"F"}, feedFormat);
var planeOutput = createVariable({prefix:"G"}, feedFormat);

// circular output
var	iOutput	= createReferenceVariable({prefix:"I"}, xyzFormat);
var	jOutput	= createReferenceVariable({prefix:"J"}, xyzFormat);
var	kOutput	= createReferenceVariable({prefix:"K"}, xyzFormat);

var cuttingMode;

function formatComment(text) {
  return String(text).replace(/[\(\)]/g, "");
}

function writeComment(text) {
  writeWords(formatComment(text));
}

function onOpen() {
  writeln(";*******************************************************");
  writeln(";HyperCube CAM post processor for Fusion360: Version 1.0");
  writeln(";Compatible with Marlin 1.1.4+");
  writeln(";For use with custom NXT bots");
  writeln(";*******************************************************");
}

/** Force output of X, Y, and Z. */
function forceXYZ() {
  xOutput.reset();
  yOutput.reset();
  zOutput.reset();
}

/** Force output of X, Y, Z, and F on next output. */
function forceAny() {
  forceXYZ();
  feedOutput.reset();
}

function onSection() {
  if(isFirstSection()) {
    writeln("");
    writeWords("M117 Starting...");
    writeWords(properties.laserOFF, "     ;Pen Up");
    writeWords("G21", "    ;Metric Values");
    writeWords("G90", "    ;Absolute Positioning");
    writeWords("G0", feedOutput.format(properties.rapidTravelXY));
    writeWords("G28")
  }
  
  if (currentSection.getType() == TYPE_JET) {
    if(currentSection.jetMode == 0) {cuttingMode = properties.laserThrough }
	else if(currentSection.jetMode == 1) {cuttingMode = properties.laserEtch }
	else if(currentSection.jetMode == 2) {cuttingMode = properties.laserVaperize }
	else {cuttingMode = (properties.laserOFF + "         ;Unknown Laser Cutting Mode") }
  }
  
  if (hasParameter("operation-comment")) {
    var comment = getParameter("operation-comment");
    if (comment) {
	  writeln("");
      writeComment("M117 " + comment);
    }
  }
}

function onDwell(seconds) {
  if (seconds > 99999.999) {
    warning(localize("Dwelling time is out of range."));
  }
  seconds = clamp(0.001, seconds, 99999.999);
  writeWords("G4 S" + seconds, "        ;Dwell time");
}

function onPower(power) {
  if (power) { writeWords(cuttingMode) }
  else { writeWords(properties.laserOFF) }
}

function onRapid(_x, _y, _z) {
  var x = xOutput.format(_x);
  var y = yOutput.format(_y);
  var z = zOutput.format(_z);
  if (x || y) {
    writeWords("G0", x, y, feedOutput.format(properties.rapidTravelXY));
  }
  if (z) {
    writeWords("G0", z, feedOutput.format(properties.rapidTravelZ));
  }
}

function onLinear(_x, _y, _z, _feed) {
  var x = xOutput.format(_x);
  var y = yOutput.format(_y);
  var z = zOutput.format(_z);
  var f = feedOutput.format(_feed);
  if(x || y || z) {
    writeWords("G1", x, y, z, f);
  }
  else if (f) {
    writeWords("G1", f);
  }
}

function onCircular(clockwise, cx, cy, cz, x, y, z, feed) {
  // one of X/Y and I/J are required and likewise
  var start = getCurrentPosition();
  
  switch (getCircularPlane()) {
  case PLANE_XY:
    writeWords(planeOutput.format(17), (clockwise ? "G2":"G3"), xOutput.format(x), yOutput.format(y), zOutput.format(z), iOutput.format(cx - start.x, 0), jOutput.format(cy - start.y, 0), feedOutput.format(feed));
    break;
  case PLANE_ZX:
    writeWords(planeOutput.format(18), (clockwise ? "G2":"G3"), xOutput.format(x), yOutput.format(y), zOutput.format(z), iOutput.format(cx - start.x, 0), kOutput.format(cz - start.z, 0), feedOutput.format(feed));
    break;
  case PLANE_YZ:
    writeWords(planeOutput.format(19), (clockwise ? "G2":"G3"), xOutput.format(x), yOutput.format(y), zOutput.format(z), jOutput.format(cy - start.y, 0), kOutput.format(cz - start.z, 0), feedOutput.format(feed));
	break;
  default:
    linearize(tolerance);
  }
}

function onSectionEnd() {
  writeWords(planeOutput.format(17));
  forceAny();
}

function onClose() {
  writeln("");
  writeWords(properties.laserOFF, "    ;Pen up");
  writeWords("G0 X0 Y0", feedOutput.format(properties.rapidTravelXY))
  writeWords("M117 Finished :)");
}
