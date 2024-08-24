# MPEG-Transport-Stream-Parser

## Overview

This project is a C++ implementation of an MPEG Transport Stream parser, specifically designed for processing a segment from a satellite stream. The parser is capable of extracting and saving audio (MP2 format) and video (H.264 format) data into separate files from a specified elementary stream identified by its PID (Packet Identifier).

The implementation also includes functionality for detecting and handling potential errors, such as packet loss.
