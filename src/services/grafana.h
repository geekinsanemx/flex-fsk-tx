/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Grafana Module - Alertmanager webhook integration
 */

#ifndef GRAFANA_H
#define GRAFANA_H

#include <Arduino.h>
#include "../core/config.h"

// =============================================================================
// FUNCTIONS (implemented in Batch 4)
// =============================================================================
void handle_grafana();
void handle_grafana_toggle();
void handle_grafana_webhook();

#endif // GRAFANA_H
