const lodash = require('./lodash.min');
const z = 30;
const w = lodash.map(lodash.range(10), function(n) { return n * 2; });

module.exports = { z, w };