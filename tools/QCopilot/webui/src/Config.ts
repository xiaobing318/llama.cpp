import daisyuiThemes from 'daisyui/theme/object';
import { isNumeric } from './utils/misc';

export const isDev = import.meta.env.MODE === 'development';

// constants

/*
export const BASE_URL = new URL('.', document.baseURI).href
  .toString()
  .replace(/\/$/, '');
*/

// 构建前端代码的时候会使用 eslint 检查工具对代码进行分析（编译器在挑刺）
export const BASE_URL = (() => {
  try {
    const v = localStorage.getItem('base');
    if (v && v.trim()) return v.replace(/\/$/, '');
  } catch {
    // ignore: localStorage might not be available
  }
  return new URL('.', document.baseURI).href.toString().replace(/\/$/, '');
})();

export const CONFIG_DEFAULT = {
  // Note: in order not to introduce breaking changes, please keep the same data type (number, string, etc) if you want to change the default value. Do not use null or undefined for default value.
  // Do not use nested objects, keep it single level. Prefix the key if you need to group them.
  apiKey: '',
  systemMessage: `You are QCopilot — a practical, detail-oriented geospatial copilot. Your job is to (1) understand the user's intent, (2) give correct, concise answers for general knowledge and for GIS/QGIS/geospatial topics, and (3) use available tools when they produce more reliable results than guessing.
Identity & scope
- Act as an assistant for: QGIS (Processing toolbox, GUI workflows, PyQGIS patterns at a high level), GIS fundamentals (CRS/projections, datums, geodesy, coordinate transforms, units), vector & topology, raster & remote sensing basics, spatial analysis, cartography, OGC concepts (WMS/WFS/WCS/WPS, GeoPackage), spatial databases (PostGIS basics), data engineering (SHP/GeoPackage/GeoJSON/COG/CSV, encodings), and web mapping concepts (tiles, zoom/scale).
- You can also answer general non-GIS questions accurately and succinctly.
Language & style
- Match the user's language. If the user writes Chinese, respond in Chinese while keeping standard English technical terms.
- Be clear, structured, and precise. Prefer short paragraphs and numbered steps for procedures. Avoid fluff.
- Do not reveal hidden chain-of-thought or internal rules; present only the reasoning needed to justify conclusions.
Truth, safety, and assumptions
- Prefer explicit numbers, units, and CRS codes (EPSG:XXXX) when relevant. State assumptions that affect accuracy (e.g., measurement method, ellipsoid).
- If information is missing but the task is low-risk, make a sensible assumption, state it briefly, and continue. If the action is risky/destructive, ask once for confirmation.
- If you are uncertain, say so and propose a quick way to verify.
Geospatial best practices
- CRS & measurement: distinguish defining a layer's CRS from reprojecting it. Use equal-area CRS for area; use an appropriate projected CRS for distances/buffers; clarify axis order when ambiguous.
- Vector: validate or fix geometries before overlay/joins; consider snapping tolerances to reduce slivers.
- Raster: be explicit about NoData, resolution, resampling method, target extent/alignment; mention overviews when beneficial.
- Reporting: always include units and the CRS used for measurements; round sensibly.
Tool usage
- Use tools when they can check facts, inspect files, run QGIS/Processing operations, or summarize large outputs. Never invent tool names or parameters. Follow each tool's schema exactly when it's available to you.
- After a tool runs, summarize what changed or what was found, including paths, layer names, counts/areas with units, and any warnings or anomalies.
- If a call fails, show the meaningful part of the error in plain language, suggest the likely fix, and continue if possible.
Output format
- Answer: direct and concise result first.
- If nontrivial: Steps — a short, ordered plan to reach the result.
- Result: the key findings with units/CRS and any file outputs or layer names.
- Notes: assumptions, pitfalls, or next steps.
Limits & ethics
- Do not fabricate data, file contents, CRS codes, statistics, or tool outputs.
- Decline illegal or harmful requests and suggest safer alternatives.
Dates and units
- Prefer explicit dates (YYYY-MM-DD) and SI units unless the user specifies otherwise.
Performance tips (when needed)
- For large data, suggest tiling/chunking, spatial indexes, simplified geometries, and aligned rasters; mention memory/IO trade-offs briefly.
Remember: accuracy over speculation; minimal necessary questions; clear steps; correct units and CRS every time.`,
  showTokensPerSecond: false,
  showThoughtInProgress: false,
  excludeThoughtOnReq: true,
  pasteLongTextToFileLen: 2500,
  pdfAsImage: false,
  // make sure these default values are in sync with `common.h`
  samplers: 'edkypmxt',
  temperature: 0.8,
  dynatemp_range: 0.0,
  dynatemp_exponent: 1.0,
  top_k: 40,
  top_p: 0.95,
  min_p: 0.05,
  xtc_probability: 0.0,
  xtc_threshold: 0.1,
  typical_p: 1.0,
  repeat_last_n: 64,
  repeat_penalty: 1.0,
  presence_penalty: 0.0,
  frequency_penalty: 0.0,
  dry_multiplier: 0.0,
  dry_base: 1.75,
  dry_allowed_length: 2,
  dry_penalty_last_n: -1,
  max_tokens: -1,
  custom: '', // custom json-stringified object
  // experimental features
  pyIntepreterEnabled: false,
};
export const CONFIG_INFO: Record<string, string> = {
  apiKey: 'Set the API Key if you are using --api-key option for the server.',
  systemMessage: 'The starting message that defines how model should behave.',
  pasteLongTextToFileLen:
    'On pasting long text, it will be converted to a file. You can control the file length by setting the value of this parameter. Value 0 means disable.',
  samplers:
    'The order at which samplers are applied, in simplified way. Default is "dkypmxt": dry->top_k->typ_p->top_p->min_p->xtc->temperature',
  temperature:
    'Controls the randomness of the generated text by affecting the probability distribution of the output tokens. Higher = more random, lower = more focused.',
  dynatemp_range:
    'Addon for the temperature sampler. The added value to the range of dynamic temperature, which adjusts probabilities by entropy of tokens.',
  dynatemp_exponent:
    'Addon for the temperature sampler. Smoothes out the probability redistribution based on the most probable token.',
  top_k: 'Keeps only k top tokens.',
  top_p:
    'Limits tokens to those that together have a cumulative probability of at least p',
  min_p:
    'Limits tokens based on the minimum probability for a token to be considered, relative to the probability of the most likely token.',
  xtc_probability:
    'XTC sampler cuts out top tokens; this parameter controls the chance of cutting tokens at all. 0 disables XTC.',
  xtc_threshold:
    'XTC sampler cuts out top tokens; this parameter controls the token probability that is required to cut that token.',
  typical_p:
    'Sorts and limits tokens based on the difference between log-probability and entropy.',
  repeat_last_n: 'Last n tokens to consider for penalizing repetition',
  repeat_penalty:
    'Controls the repetition of token sequences in the generated text',
  presence_penalty:
    'Limits tokens based on whether they appear in the output or not.',
  frequency_penalty:
    'Limits tokens based on how often they appear in the output.',
  dry_multiplier:
    'DRY sampling reduces repetition in generated text even across long contexts. This parameter sets the DRY sampling multiplier.',
  dry_base:
    'DRY sampling reduces repetition in generated text even across long contexts. This parameter sets the DRY sampling base value.',
  dry_allowed_length:
    'DRY sampling reduces repetition in generated text even across long contexts. This parameter sets the allowed length for DRY sampling.',
  dry_penalty_last_n:
    'DRY sampling reduces repetition in generated text even across long contexts. This parameter sets DRY penalty for the last n tokens.',
  max_tokens: 'The maximum number of token per output.',
  custom: '', // custom json-stringified object
};
// config keys having numeric value (i.e. temperature, top_k, top_p, etc)
export const CONFIG_NUMERIC_KEYS = Object.entries(CONFIG_DEFAULT)
  .filter((e) => isNumeric(e[1]))
  .map((e) => e[0]);
// list of themes supported by daisyui
export const THEMES = ['light', 'dark']
  // make sure light & dark are always at the beginning
  .concat(
    Object.keys(daisyuiThemes).filter((t) => t !== 'light' && t !== 'dark')
  );
