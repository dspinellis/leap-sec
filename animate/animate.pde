import java.util.Map;
import java.math.*;
import processing.data.Table;

final int fontSize = 22;
final int leftMargin = 10;
final int topMargin = 120;
final int columnWidth = 290;
final int dataColumn = leftMargin + columnWidth / 2;
final boolean saveFrames = false;
double virtualFrameRate = 25;
final int textHeight = 40;
PFont fontNormal, fontBold;
float textWidth;
// Y offsets for displaying values
int currentValues;
int historyValues;
// History scroll offset
int scrollOffset = -1;
// Details of systems we process
Table systems;
// Values over time
Table v;
// Table value to be processed
int currentRow;

// The x coordinate of each system displayed
HashMap<String,Integer> systemColumn = new HashMap<String,Integer>();

// The past 4 seconds for each system tracked
ArrayList<HashMap<String, TableRow>> history = new ArrayList<HashMap<String, TableRow>>();
final int historyRows = 3;

// Return the fractional part of a number
float
frac(float x)
{
  return x - floor(x);
}

void
setup()
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

  // Draw table headings
  textFont(fontBold);
  fill(color(0, 0, 255));
  // Table column headings
  systems = loadTable("c:/dds/src/fun/leap-sec/systems.txt", "tsv,header");
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
  currentValues = y;
  text("SI", leftMargin, y);
  y += textHeight;
  text("Unix", leftMargin, y);
  y += textHeight;
  text("Human", leftMargin, y);
  historyValues = y + 2 * textHeight;

  textFont(fontNormal);
  if (saveFrames) {
    /*
     * Virtually, iterate over all frames to ensure that no frames
     * are skipped, as could be the case in real-time frame processing.
     */
    noLoop();
    for (TableRow r : v.rows()) {
      processRow(r);
      float t = r.getFloat("abs");
      if (t < frameCount / virtualFrameRate)
        continue;
      frameCount++;
      saveFrame("r:/frames/#######.png");
    }
    exit();
  }
}

// Process a new data row
void
processRow(TableRow r)
{
  displayCurrentValues(currentValues, r);
  displayHistory();
  updateHistory(r);
}

void
draw()
{
  for (;;) {
    TableRow r = v.getRow(currentRow);
    float tData = r.getFloat("abs");
    float tFrame = frameCount / frameRate;
    // Return if not yet time to process this row
    if (tData > tFrame)
      return;
    // Don't process stale data
    if (tData >= tFrame - 1. / frameRate)
      processRow(r);
    currentRow++;
    if (currentRow >= v.getRowCount()) {
      noLoop();
      return;
    }
  }
}

// Update the history as needed
void
updateHistory(TableRow r)
{
  float t = r.getFloat("abs");
  if (frac(t) < 0.5)
    return;

  if (history.size() == 0)
    history.add(new HashMap<String, TableRow>());

  HashMap<String, TableRow> lastRow = history.get(history.size() - 1);
  String name = r.getString("system");
  TableRow lastVal = lastRow.get(name);
  if (lastVal == null) {
    // This system is not yet in the last row; add it
    // println("Adding value " + name + " at " + t + " to history");
    lastRow.put(name, r);
    // See if we just completed a new row and we can start scrolling it in
    if (lastRow.size() == systems.getRowCount())
      scrollOffset = 0;
  } else if (floor(lastVal.getFloat("abs")) < floor(t)) {
    // The last row has outdated data; add a new row
    // println("Adding new history row " + name + " t=" + t + " after t=" + lastVal.getFloat("abs"));
    lastRow = new HashMap<String, TableRow>();
    history.add(lastRow);
    if (history.size() > historyRows + 1)
      history.remove(0);
    lastRow.put(name, r);
  }
}

void
displayHistory()
{
  int y = historyValues;
  for (HashMap<String, TableRow> tr : history) {
    for (Map.Entry me : tr.entrySet()) {
      displayHistoryValues(y, (TableRow)me.getValue());
    }
    y += 3 * textHeight;
  }
}

void
displayCurrentValues(int y, TableRow r)
{
  String name = r.getString("system");
  // println(name);
  int x = systemColumn.get(name);

  // Clear area
  fill(255);
  stroke(255);
  rect(x, y - textHeight, columnWidth, textHeight * 3);

  fill(0);

  text(r.getString("abs"), x, y);
  y += textHeight;
  text(r.getString("unix"), x, y);
  y += textHeight;
  text(r.getString("fdate") + " " + r.getString("ftime"), x, y);
}

void
displayHistoryValues(int y, TableRow r)
{
  String name = r.getString("system");
  // println(name);
  int x = systemColumn.get(name);

  // Clear area
  fill(255);
  stroke(255);
  rect(x, y - textHeight, columnWidth, textHeight * 2);
  rect(leftMargin, y - textHeight, textWidth * 3, textHeight);

  fill(0);

  text(floor(r.getFloat("abs")), leftMargin, y);
  // Double precision needed here
  int ut = (int)Math.floor(Double.parseDouble(r.getString("unix"))); 
  text(ut, x, y);
  y += textHeight;
  text(r.getString("fdate") + " " + r.getString("ftime"), x, y);
}