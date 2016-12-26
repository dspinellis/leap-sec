import java.util.Map;
import processing.data.Table;

final int fontSize = 22;
final int leftMargin = 10;
final int topMargin = 120;
final int columnWidth = 290;
final int dataColumn = leftMargin + columnWidth / 2;
double virtualFrameRate = 25;
final int textHeight = 40;
PFont fontNormal, fontBold;
float textWidth;
int timeSI;
int timeUnix;
int timeHuman;

// Values over time
Table v;

// Frame at which to show next character
double updateFrame;
import java.util.Map;

HashMap<String,Integer> systemColumn = new HashMap<String,Integer>();

void setup()
{
  // Normal 
  fontNormal = createFont("c:/Windows/Fonts/LucidaSansTypewriter.ttf", fontSize);
  // Bold
  fontBold = createFont("c:/Windows/Fonts/LucidaSansTypewriter-Bd.ttf", fontSize);

  v = loadTable("c:/dds/src/fun/leap-sec/all.txt", "tsv,header");
  textWidth = textWidth("m");
  background(255);
  // Compatible with the video we're recording
  size(1920, 1080);
  frameRate((int)virtualFrameRate);
  noLoop();

  // Draw table headings
  textFont(fontBold);
  fill(color(0, 0, 255));
  // Table column headings
  Table systems = loadTable("c:/dds/src/fun/leap-sec/systems.txt", "tsv,header");
  int x = dataColumn;
  int y = topMargin;
  for (TableRow row : systems.rows()) {
    systemColumn.put(row.getString("name"), x);
    text(row.getString("OS"), x, y);
    text(row.getString("variant"), x, y + textHeight - 5);
    x += columnWidth;
  }
  // Table row headings
  y += 2 * textHeight;
  text("SI", leftMargin, timeSI = y);
  y += textHeight;
  text("Unix", leftMargin, timeUnix = y);
  y += textHeight;
  text("Human", leftMargin, timeHuman = y);
  
  
  textFont(fontNormal);
  for (int i = 0; i < v.getRowCount(); i++) {
    myDraw(i);
    if (v.getRow(i).getFloat("abs") < frameCount / virtualFrameRate)
      continue;
    frameCount++;
    if (v.getRow(i).getFloat("abs") > 500)
      break;
    saveFrame("r:/frames/#######.png");
  }
  exit();
}

void myDraw(int rowNum)
{
  TableRow r = v.getRow(rowNum);
  String name = r.getString("system");
  println(name);
  int x = systemColumn.get(name);

  // Clear area
  fill(255);
  stroke(255);
  rect(x, timeSI - textHeight, columnWidth, textHeight * 3);
  
  fill(0);

  text(r.getString("abs"), x, timeSI);
  text(r.getString("unix"), x, timeUnix);
  text(r.getString("fdate") + " " + r.getString("ftime"), x, timeHuman);
}