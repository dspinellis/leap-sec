/*-
 * Create an animation from a sorted time advancement log
 *
 * Copyright 2016 Diomidis Spinellis
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

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
final String cc = "CC BY 4.0 - www.spinellis.gr/blog/20170101";

// The absolute time for the beginning and end records
double beginRecordsBegin = -1;
double endRecordsBegin = -1;

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
      float t = animationTime(r);
      println("Working on frame=" + frameCount + " t=" + t);
      if (t < frameCount / virtualFrameRate) {
        processRow(r);
        continue;
      }
      // Save current state, before processing row from a different time
      frameCount++;
      saveFrame("r:/frames/#######.png");
      processRow(r);
    }
    exit();
  }
}

/* Return the time in the animation for the specified table row
 * First 12 hours (-b) play in 28.8s
 * Next 2 minutes (m) play in 2m
 * Last 12 hours (-e) play in 28.8s
 * Return -1 for records to be skipped
 */
float
animationTime(TableRow r)
{
  String recordType = r.getString("type");
  String fTime = r.getString("ftime");
  double t = Double.parseDouble(r.getString("abs"));

  // println("type=" + recordType + " ftime=" + fTime);
  if (t == 2085978496)  // Skip invalid records
    return -1;
  if (recordType.equals("-b")) {        // Begin records; one per minute
    if (match(fTime, "^12:00") != null)
      return -1;
    if (beginRecordsBegin == -1 && match(fTime, "^12:01") != null)
      beginRecordsBegin = Math.floor(t);
    return (float)((t - beginRecordsBegin) / 60 / virtualFrameRate);
  } else if (recordType.equals("m")) {  // Midnight records; one per second
    return (float)(28.8 + t);
  } else if (recordType.equals("-e")) {        // End records; one per minute
    if (match(fTime, "^00:00") != null)
      return -1;
    if (endRecordsBegin == -1 && match(fTime, "^00:01") != null) {
      endRecordsBegin = Math.floor(t);
      clearHistory();
    }
    return (float)(28.8 + 122 + (t - endRecordsBegin) / 60 / virtualFrameRate);
  }
  return 0f;
}

// Process a new data row
void
  processRow(TableRow r)
{
  if (r.getFloat("abs") < 200) {
    displayCurrentValues(currentValues, r, false);
    displayHistory();
    updateHistory(r);
  } else
    displayCurrentValues(currentValues, r, true);
}

void
  draw()
{
  for (;;) {
    if (currentRow >= v.getRowCount()) {
      noLoop();
      return;
    }
    
    TableRow r = v.getRow(currentRow);
    float tData = animationTime(r);
    float tFrame = frameCount / frameRate;

    if (tData == -1) {
      currentRow++;
      return;
    }
    // println("tData=" + tData + " tFrame=" + tFrame);
    // Return if not yet time to process this row
    if (tData > tFrame)
      return;
    // Don't process stale data
    if (tData >= tFrame - 1. / frameRate)
      processRow(r);
    currentRow++;
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
clearHistory()
{
  fill(255);
  stroke(255);
  rect(leftMargin, historyValues - 3 * textHeight, width, historyEntryHeight * (historyRows + 1));
  fill(0);
}

void
displayHistory()
{
  clearHistory();
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
  displayCurrentValues(int y, TableRow r, boolean showDelta)
{
  String name = r.getString("system");
  // println(name);
  int x = systemColumn.get(name);

  // Clear area
  fill(255);
  stroke(255);
  rect(x, y - textHeight, columnWidth, textHeight * 4);

  fill(0);

  text(r.getString("abs"), x, y);
  y += textHeight;
  text(r.getString("unix"), x, y);
  y += textHeight;
  text(r.getString("fdate") + " " + r.getString("ftime"), x, y);
  if (showDelta) {
    y += textHeight;

    //textFont(fontBold);
    fill(headingColor);
    text("Delta-T", leftMargin, y);
    textFont(fontNormal);
    fill(0);

    float delta = (float)(Double.parseDouble(r.getString("abs")) -
      Double.parseDouble(r.getString("unix")));
    text(delta, x, y);
  }
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
