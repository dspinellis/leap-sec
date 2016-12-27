import java.util.Map;
import processing.data.Table;

final boolean saveFrames = true;
final int fontSize = 22;
final int leftMargin = 10;
final int topMargin = 120;
final int columnWidth = 290;
final int dataColumn = leftMargin + columnWidth / 2;
double virtualFrameRate = 25;
final int textHeight = 40;
final int historyEntryHeight = (int)(2.5 * textHeight);
PFont fontNormal, fontBold;
float textWidth;
// Y offsets for displaying values
int currentValues;
int historyValues;
// History scroll offset
int scrollOffset;
// Details of systems we process
Table systems;
// Values over time
Table v;
// Table value to be processed
int currentRow;
final color headingColor = color(64, 120, 192);

// The x coordinate of each system displayed
HashMap<String, Integer> systemColumn = new HashMap<String, Integer>();

// The past seconds for each system tracked
ArrayList<HashMap<String, TableRow>> history = new ArrayList<HashMap<String, TableRow>>();
final int historyRows = 6;
final String title = "Leap Second Log: 2016-12-31";
final String cc = "CC BY 4.0 www.spinellis.gr";

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

  // Video headings
  fill(color(128));
  textFont(createFont("c:/Windows/Fonts/LucidaSansTypewriter.ttf", fontSize * 2));
  text(title, (width - textWidth(title)) / 2, textHeight);
  textFont(fontNormal);
  text(cc, width - textWidth(cc) - 5, height - 5);

  // Draw table headings
  textFont(fontBold);
  fill(headingColor);
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
  historyValues = y + 3 * textHeight;

  textFont(fontNormal);
  if (saveFrames) {
    /*
     * Virtually, iterate over all frames to ensure that no frames
     * are skipped, as could be the case in real-time frame processing.
     */
    noLoop();
    // Do not use (TableRow r : v.rows()), because r returns a static area
    // that is invaldiated after every iteration
    for (int i = 0; i < v.getRowCount(); i++) {
      TableRow r = v.getRow(i);
      float t = r.getFloat("abs");
      println("Working on frame=" + frameCount + " t=" + t);
      processRow(r);
      if (t < frameCount / virtualFrameRate)
        continue;
      frameCount++;
      saveFrame("r:/frames/#######.png");
      // if (frameCount > 245) break;
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
  for (;; ) {
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
  } else if (floor(lastVal.getFloat("abs")) < floor(t)) {
    // The last row has outdated data; add a new row
    // println("Adding new history row " + name + " t=" + t + " after t=" + lastVal.getFloat("abs"));
    scrollOffset = 0;
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
  // Clear previous area
  fill(255);
  stroke(255);
  rect(leftMargin, historyValues - 3 * textHeight, width, historyEntryHeight * (historyRows + 1));
  fill(0);

  int y = historyValues;
  for (HashMap<String, TableRow> tr : history) {
    for (Map.Entry me : tr.entrySet()) {
      displayHistoryValues(y - scrollOffset, (TableRow)me.getValue());
    }
    y += historyEntryHeight;
  }
  if (history.size() == historyRows + 1 && scrollOffset < historyEntryHeight)
    scrollOffset++;

  // Clear area outside the scroll clipping window
  fill(255);
  stroke(255);
  rect(leftMargin, historyValues - 3 * textHeight, width, textHeight * 2);
  rect(leftMargin, historyValues + historyRows * historyEntryHeight - textHeight, width, textHeight * 2);

  // Draw label
  textFont(fontBold);
  fill(headingColor);
  text("SI", leftMargin, historyValues - textHeight * 1.5);
  textFont(fontNormal);
  fill(0);
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
  int x = systemColumn.get(name);

  if (r.getString("ftime").equals("00:00:00"))
    fill(color(255, 0, 0));
  text(floor(r.getFloat("abs")), leftMargin, y);
  // Double precision needed here
  int ut = (int)Math.floor(Double.parseDouble(r.getString("unix"))); 
  text(ut, x, y);
  y += textHeight;
  text(r.getString("fdate") + " " + r.getString("ftime"), x, y);
  fill(0);
}