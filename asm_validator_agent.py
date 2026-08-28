#!/usr/bin/env python3
"""
CPUSim64 Assembly Validator Agent

A LangGraph-based agent that validates .asm files against CPUSim64 documentation,
assembles them, and runs the simulation. Uses Ollama with muse-glimmer:30b-mlx.

Usage:
    python asm_validator_agent.py <directory_of_asm_files>
"""

import os
import sys
import glob
import subprocess
from pathlib import Path
from typing import TypedDict, Annotated, Sequence

from langchain_ollama import ChatOllama
from langchain_core.messages import BaseMessage, HumanMessage, SystemMessage, AIMessage
from langchain_core.tools import tool
from langgraph.graph import StateGraph, END
from langgraph.prebuilt import ToolNode
from langgraph.graph.message import add_messages


# --- Configuration ---
SCRIPT_DIR = Path(__file__).resolve().parent
DOC_DIR = SCRIPT_DIR / "cpusim64" / "src" / "main" / "resources" / "documentation"
EXAMPLES_DIR = DOC_DIR / "examples"
ASSEMBLE_SCRIPT = SCRIPT_DIR / "assemble.sh"
RUN_SCRIPT = SCRIPT_DIR / "run.sh"

# Documentation files to load for context
DOC_FILES = [
    "Architecture.html",
    "Directive_Reference.html",
    "Instruction_Set_Reference.html",
    "Interrupt_Reference.html",
    "Library_Reference.html",
    "Programmer_Guide.html",
]


# --- State Definition ---
class AgentState(TypedDict):
    messages: Annotated[Sequence[BaseMessage], add_messages]
    current_file: str
    files_to_process: list[str]
    results: dict[str, dict]


# --- Load Documentation ---
def load_documentation() -> str:
    """Load all HTML documentation files and strip HTML tags for plain text."""
    import re

    docs = []
    for filename in DOC_FILES:
        filepath = DOC_DIR / filename
        if filepath.exists():
            content = filepath.read_text(encoding="utf-8")
            # Strip HTML tags for cleaner LLM context
            text = re.sub(r"<[^>]+>", "", content)
            text = re.sub(r"\s+", " ", text).strip()
            docs.append(f"=== {filename} ===\n{text}\n")
    return "\n".join(docs)


def load_examples() -> str:
    """Load a selection of example .asm files for reference."""
    examples = []
    example_files = [
        "hello_world.asm",
        "example_function.asm",
        "example_macro.asm",
        "example_for.asm",
        "example_while.asm",
        "example_cond.asm",
        "example_io.asm",
        "example_string.asm",
    ]
    for filename in example_files:
        filepath = EXAMPLES_DIR / filename
        if filepath.exists():
            content = filepath.read_text(encoding="utf-8")
            examples.append(f"=== {filename} ===\n{content}\n")
    return "\n".join(examples)


# --- Tools ---
@tool
def read_asm_file(filepath: str) -> str:
    """Read the contents of a .asm file."""
    path = Path(filepath)
    if not path.exists():
        return f"ERROR: File not found: {filepath}"
    if not path.suffix == ".asm":
        return f"ERROR: Not an .asm file: {filepath}"
    return path.read_text(encoding="utf-8")


@tool
def assemble_file(filepath: str) -> str:
    """Assemble a .asm file using the CPUSim64 assembler.
    Returns the assembler output (stdout + stderr).
    The filepath should be the full path to the .asm file."""
    path = Path(filepath)
    if not path.exists():
        return f"ERROR: File not found: {filepath}"

    # assemble.sh expects basename without .asm extension
    basename = path.stem
    working_dir = path.parent

    try:
        result = subprocess.run(
            [str(ASSEMBLE_SCRIPT), basename],
            capture_output=True,
            text=True,
            timeout=30,
            cwd=str(working_dir),
        )
        output = ""
        if result.stdout:
            output += f"STDOUT:\n{result.stdout}\n"
        if result.stderr:
            output += f"STDERR:\n{result.stderr}\n"
        output += f"EXIT CODE: {result.returncode}\n"

        # Check if .o64 was created
        obj_dir = working_dir / "obj"
        o64_file = obj_dir / f"{basename}.o64"
        if o64_file.exists():
            output += "Assembly SUCCESSFUL - .o64 file created.\n"
        else:
            output += "Assembly FAILED - no .o64 file produced.\n"

        return output if output.strip() else "Assembly completed with no output."
    except subprocess.TimeoutExpired:
        return "ERROR: Assembly timed out after 30 seconds."
    except Exception as e:
        return f"ERROR: {str(e)}"


@tool
def run_file(filepath: str) -> str:
    """Assemble and run a .asm file using the CPUSim64 simulator.
    Returns the simulation output (stdout + stderr).
    The filepath should be the full path to the .asm file."""
    path = Path(filepath)
    if not path.exists():
        return f"ERROR: File not found: {filepath}"

    # run.sh expects basename without .asm extension
    basename = path.stem
    working_dir = path.parent

    try:
        result = subprocess.run(
            [str(RUN_SCRIPT), basename],
            capture_output=True,
            text=True,
            timeout=60,
            cwd=str(working_dir),
        )
        output = ""
        if result.stdout:
            output += f"STDOUT:\n{result.stdout}\n"
        if result.stderr:
            output += f"STDERR:\n{result.stderr}\n"
        output += f"EXIT CODE: {result.returncode}\n"
        return output if output.strip() else "Simulation completed with no output."
    except subprocess.TimeoutExpired:
        return "ERROR: Simulation timed out after 60 seconds."
    except Exception as e:
        return f"ERROR: {str(e)}"


# --- Build the Graph ---
def build_graph():
    """Build the LangGraph agent."""

    # Load documentation once
    print("Loading CPUSim64 documentation...")
    documentation = load_documentation()
    examples = load_examples()

    system_prompt = f"""You are a CPUSim64 assembly language expert. Your job is to validate .asm files
for correctness, assemble them, and run them.

For each .asm file you process, follow these steps:
1. Read the file contents using read_asm_file
2. Analyze the code against the CPUSim64 documentation to check for:
   - Correct instruction usage and syntax
   - Proper use of directives and preprocessor commands
   - Valid register names and addressing modes
   - Correct label formatting
   - Proper include statements
   - Logical correctness of the program
3. Assemble the file using assemble_file to verify it compiles
4. If assembly succeeds, run the file using run_file to check execution
5. Provide a summary report with:
   - Whether the file conforms to CPUSim64 syntax
   - Any issues found (errors, warnings, suggestions)
   - Assembly result (pass/fail)
   - Execution result and output

Here is the CPUSim64 documentation for reference:

{documentation}

Here are example programs showing correct CPUSim64 assembly:

{examples}
"""

    # Initialize the LLM
    llm = ChatOllama(
        model="muse-glimmer:30b-mlx",
        temperature=0,
    )

    tools = [read_asm_file, assemble_file, run_file]
    llm_with_tools = llm.bind_tools(tools)

    def agent_node(state: AgentState) -> dict:
        """The agent reasoning node."""
        messages = state["messages"]
        # Prepend system message if not already there
        if not messages or not isinstance(messages[0], SystemMessage):
            messages = [SystemMessage(content=system_prompt)] + list(messages)
        response = llm_with_tools.invoke(messages)
        return {"messages": [response]}

    def should_continue(state: AgentState) -> str:
        """Determine whether to call tools or finish."""
        last_message = state["messages"][-1]
        if hasattr(last_message, "tool_calls") and last_message.tool_calls:
            return "tools"
        return END

    # Build the graph
    workflow = StateGraph(AgentState)

    # Add nodes
    workflow.add_node("agent", agent_node)
    workflow.add_node("tools", ToolNode(tools))

    # Set entry point
    workflow.set_entry_point("agent")

    # Add edges
    workflow.add_conditional_edges("agent", should_continue, {"tools": "tools", END: END})
    workflow.add_edge("tools", "agent")

    return workflow.compile()


def process_directory(directory: str):
    """Process all .asm files in the given directory."""
    dir_path = Path(directory).resolve()
    if not dir_path.exists():
        print(f"ERROR: Directory not found: {directory}")
        sys.exit(1)
    if not dir_path.is_dir():
        print(f"ERROR: Not a directory: {directory}")
        sys.exit(1)

    # Find all .asm files
    asm_files = sorted(glob.glob(str(dir_path / "*.asm")))
    if not asm_files:
        print(f"No .asm files found in: {directory}")
        sys.exit(1)

    print(f"Found {len(asm_files)} .asm file(s) to validate:")
    for f in asm_files:
        print(f"  - {Path(f).name}")
    print()

    # Build the agent graph
    graph = build_graph()

    # Process each file
    for asm_file in asm_files:
        print(f"{'='*60}")
        print(f"Processing: {Path(asm_file).name}")
        print(f"{'='*60}")

        initial_state = {
            "messages": [
                HumanMessage(
                    content=f"Please validate, assemble, and run the following CPUSim64 assembly file: {asm_file}"
                )
            ],
            "current_file": asm_file,
            "files_to_process": asm_files,
            "results": {},
        }

        # Run the agent
        final_state = graph.invoke(initial_state)

        # Print the final response
        for message in final_state["messages"]:
            if isinstance(message, AIMessage) and not message.tool_calls:
                print(f"\n{message.content}\n")

    print(f"\n{'='*60}")
    print("Validation complete.")
    print(f"{'='*60}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <directory_of_asm_files>")
        print(f"\nExample: {sys.argv[0]} ./my_programs")
        sys.exit(1)

    process_directory(sys.argv[1])
