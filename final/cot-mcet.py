import os
import json
import requests
from plantuml import PlantUML

import re  # if not already imported

# File that contains the textual UMLs (your example JSON)
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
JSON_RESULTS_FILE = os.path.join(BASE_DIR, "cot_results.json")

def slugify(title: str) -> str:
    """Turn a title into a safe folder name."""
    s = title.strip().lower()
    s = re.sub(r'[^a-z0-9]+', '_', s)
    return s.strip('_') or "example"

all_descriptions = [
    {
        "title": "Project Management System",
        "desc": "A project manager uses the project management system to manage a project. The project manager leads a team to execute the project within the project's start and end dates. Once a project is created in the project management system, a manager may initiate and later terminate the project due to its completion or for some other reason. As input, a project uses requirements. As output, a project produces a system (or part of a system). The requirements and system are work products: things that are created, used, updated, and elaborated on throughout a project. Every work product has a description, is of some percent complete throughout the effort, and may be validated. However, validation is dependent on the type of work product. For example, the requirements are validated with users in workshops, and the system is validated by being tested against the requirements. Furthermore, requirements may be published using various types of media, including on an intranet or in paper form; and systems may be deployed onto specific platforms."
    },
    {
        "title": "Hollywood Approach",
        "desc": "We are interested in building a software application to manage filmed scenes for realizing a movie, by following the so-called “Hollywood Approach”. Every scene is identified by a code (a string) and it is described by a text in natural language. Every scene is filmed from different positions (at least one), each of this is called a setup. Every setup is characterized by a code (a string) and a text in natural language where the photographic parameters are noted (e.g., aperture, exposure, focal length, filters, etc.). Note that a setup is related to a single scene. For every setup, several takes may be filmed (at least one). Every take is characterized by a (positive) natural number, a real number representing the number of meters of film that have been used for shooting the take, and the code (a string) of the reel where the film is stored. Note that a take is associated to a single setup. Scenes are divided into internals that are filmed in a theater, and externals that are filmed in a location and can either be “day scene” or “night scene”. Locations are characterized by a code (a string) and the address of the location, and a text describing them in natural language."
    },
    {
        "title": "Word Processor",
        "desc": "A user can open a new or existing document. Text is entered through a keyboard. A document is made up of several pages and each page is made up of a header, body and footer. Date, time and page number may be added to header or footer. Document body is made up of sentences, which are themselves made up of words and punctuation characters. Words are made up of letters, digits and/or special characters. Pictures and tables may be inserted into the document body. Tables are made up of rows and columns and every cell in a table can contain both text and pictures. Users can save or print documents."
    },
    {
        "title": "Patient Record and Scheduling System",
        "desc": "A patient record and scheduling system in a doctor’s office is used by the receptionists, nurses, and doctors. The receptionists use the system to enter new patient information when first-time patients visit the doctor. They also schedule all appointments. The nurses use the system to keep track of the results of each visit including diagnosis and medications. For each visit, free form text fields are used captures information on diagnosis and treatment. Multiple medications may be prescribed during each visit. The nurses can also access the information to print out a history of patient visits. The doctors primarily use the system to view patient history. The doctors may enter some patient treatment information and prescriptions occasionally, but most frequently they let the nurses enter this information. -- Each patient is assigned to a family. The head of family is responsible for the person with the primary medical coverage. Information about doctors is maintained since a family has a primary care physician, but different doctors may be the ones seeing the patient during the visit."
    },
    {
        "title": "Movie-Shop",
        "desc": "♣ Design a system for a movie-shop, in order to handle ordering of movies and browsing of the catalogue of the store, and user subscriptions with rechargeable cards. ♣ Only subscribers are allowed hiring movies with their own card. ♣ Credit is updated on the card during rent operations. ♣ Both users and subscribers can buy a movie and their data are saved in the related order. ♣ When a movie is not available it is ordered ."
    },
    {
        "title": "Flights",
        "desc": "We want to model a system for management of flights and pilots. An airline operates flights. Each airline has an ID. Each flight has an ID a departure airport and an arrival airport: an airport as a unique identifier. Each flight has a pilot and a co-pilot, and it uses an aircraft of a certain type; a flight has also a departure time and an arrival time. An airline owns a set of aircrafts of different types. An aircraft can be in a working state or it can be under repair. In a particular moment an aircraft can be landed or airborne. A company has a set of pilots: each pilot has an experience level: 1 is minimum, 3 is maximum. A type of aeroplane may need a particular number of pilots, with a different role (e.g.: captain, co-pilot, navigator): there must be at least one captain and one co-pilot, and a captain must have a level 3."
    },
    {
        "title": "Bank System",
        "desc": "A bank system contains data on customers (identified by name and address) and their accounts. Each account has a balance and there are 2 type of accounts: one for savings which offers an interest rate, the other for investments, used to buy stocks. Stocks are bought at a certain quantity for a certain price (ticker) and the bank applies commission on stock orders."
    },
    {
        "title": "Veterinary Clinic",
        "desc": "The owner of a veterinary clinic wants to create a database to store information about all veterinary services performed. After some research he came up with the following requirements: ● For each admitted animal, its name, breed (if any) and owner must be stored. Each animal should be given an unique numeric identifier. ● For each owner, its name, address and phone number should be stored. An unique numeric identifier should also be generated for each one of them. ● An animal might be owner-less. This happens frequently as the clinic often rescues abandoned dogs from the streets in order to treat them and get them new owners. ● It should be possible to store information about a specific breed even if no animals of that breed have been treated at the clinic. ● Each appointement always has a responsible physician. All appointements start at a certain date and time; and are attended by an animal (and of course its owner). ● For each physician, his name, address and phone number should be stored. An unique numeric identifier should also be generated for each one of them. ● In an appointement, several medical conditions might be detected. Each condition has a common name and a scientific name. No two conditions have the same scientific name. ● It should be possible to store information about the most common conditions for each different breed in the database."
    },
    {
        "title": "Auto Repair",
        "desc": "An auto repair shop, that sells and mounts parts and accessories for all kinds of vehicles, wants a new information system to manage their clients, parts, accessories and assembly services: ● There are several employees. Each one of them has an unique identifying number, a name and an address. ● In this shop, assembly services, where parts and accessories are installed in a vehicle, are executed. For each one these services the following data must be stored: In which car the service was executed, how many kms had the car at the time, who was the responsible employee, which parts and accessories were fitted, how many work hours did it take and the admission and finish dates. ● Parts and accessories are only sold together with an assembly service. ● Each part/accessory only fits in some car models. Therefore, it is important to store that information. ● Each part/accessory has a category (radio, tyre, …), a serial number and a price. ● Each car has a license plate, a make, a model, a color and an owner. Each owner has a name, identifying number, address and a phone. ● One person can own more than one car but one car only has one owner."
    },
    {
        "title": "Restaurant",
        "desc": "The owner of a small restaurant wants a new information system to store data for all meals consumed there and also to keep a record of ingredients kept in stock. After some research he reached the following requirements list: ● Each ingredient has a name, a measuring unit (e.g. olive oil is measured in liters, while eggs are unit based) and a quantity in stock. There are no two ingredients with the same name. ● Each dish is composed of several ingredients in a certain quantity. An ingredient can, of course, be used in different dishes. ● A dish has an unique name and a numeric identifier. ● There are several tables at the restaurant. Each one of them has an unique numeric identifier and a maximum ammount of people that can be seated there. ● In each meal, several dishes are consumed at a certain table. The same dish can be eaten more than once in the same meal. ● A meal takes place in a certain date and has a start and end time. Each meal has a responsible waiter. ● A waiter has an unique numerical identifier, a name, an address and a phone number. ● In some cases it is important to store information about the client that consumed the meal. A client has a tax identification number, a name and an address."
    },
    {
        "title": "Deliveries",
        "desc": "The owner of a small delivery company plans to have an information system that allows him to save data about his customers and deliveries. After some time studying the problem, he reached the following requirements: ● Each customer has a VAT number, a name, a phone number and an address. There are no two clients with the same VAT number. ● When a customer wants to send a package to another customer, he just has to login to the company website, select the customer he wants to send the package to, enter the package’s weight and if the delivery is normal or urgent. He then receives an unique identifier code that he writes on the package. ● The package is then delivered by the customer at the delivery center of his choosing. A delivery center has a unique name and an address. ● Each client has an associated delivery center. This delivery center is chosen by the company and it is normally the one closest to the customer’s house. ● The package is them routed through an internal system until it reaches the delivery center of the recipient. ● The package is then delivered by hand from that delivery center to the recipient by a courier. ● Couriers have a single VAT number, a name and a phone number. Each courier works in a single delivery center. ● A courier is assigned to a packet as soon as the packet is introduced in the system."
    },
    {
        "title": "Furniture",
        "desc": "The known furniture factory Hi-Key-Ah, intends to implement an information system to store all data on the different types of furniture and components it produces: ● The factory produces several lines of furniture, each with a different name and consisting of several pieces of furniture of different types (beds, tables, chairs, …). ● All furniture pieces have a type, a single reference (eg CC6578) and a selling price. ● The major competitive advantage of this innovative plant is the fact that each component produced can be used in more than one piece of furniture. ● Each piece of furniture is thus composed of several components. The same component can be used more than once in the same piece. ● Every type of component produced is assigned a unique numerical code, a manufacturing price and a type (screw, hinge, shelf …). ● The furniture is then sold in various stores throughout the world. Each store has a different address and a fax number. ● To make the manufacturing process more efficient, stores have to place orders everytime they need to replenish their stock. These orders must also be stored in the database. ● Each order has a order number, a date, the store that placed the order as well as a list of all the ordered furniture and their quantities."
    },
    {
        "title": "Factory",
        "desc": "Create a database for a factory with the following requirements. Don’t forget to add unique identifiers for each one of the entities if needed. ● A factory has several machines. Each one of them is operated by several workers. ● A worker might work in more than one machine. ● In this factory, several products of different types, are produced. Each different type of product is produced in a single machine. But, the same machine can produce more than one type of product. ● Products from the same type are all produced from the same single material and have the same weigth. ● Clients can issue purchase orders. Each order has a list of the desired products and their quantity. ● For each worker, the following data should be stored in the database: name (first and last), birth date, address and a list of his skills. ● For each machine, the following data should be stored: serial number, make, model and purchase date. ● For each client, the followig data should be stored: name, address, phone number and name of the contact person (if any). ● For each purchase order, the following date should be stored: order number, date it has been made, expected and actual delivery date."
    },
    {
        "title": "Bycicle Rental",
        "desc": "A bicycle renting company wants to create an information system that allows it to store the data regarding all their reservations and rentals. The system should follow these requirements: ● It should be possible to store the national id number (NIN), tax identification number (TIN), name and address for every client. The NIN and TIN must be different for every client and all clients should have at least a TIN and a name. ● The database should also contain information about the bicycle models that can be rented- Each model has an unique name, a type (that can only be road, mountain, bmx or hybrid) and the number of gears. ● Each bicycle has a unique identifying number and a model. ● The company has several different stores where bicycles can be picked up and returned. Each one of these stores is identified by an unique name and has an address (both mandatory). ● When a reservation is made, the following data must be known: which client did the reservation, when will he pick up the bike (day), which bike model he wants and where will he pick up the bike (store). ● When a bike is picked up, the actual bike that was picked up must be stored in the database. ● When a bike is returned, the return date should be stored in the database."
    },
    {
        "title": "Saturn Int. Management",
        "desc": "Saturn Int. management wants to improve their security measures, both for their building and on site. They would like to prevent people who are not part of the company to use their car park. Saturn Int. has decided to issue identity cards to all employees. Each card records the name, department and number of a company staff, and give them access to the company car park. Employees are asked to wear the cards while on the site. There is a barrier and a card reader placed at the entrance to the car park. When a driver drives his car into the car park, he/she inserts his or her identity card into the card reader. The card reader then verify the card number to see if it is known to the system. If the number is recognized, the reader sends a signal to trigger the barrier to rise. The driver can then drive his/her car into the car park. There is another barrier at the exit of the car park, which is automatically raised when a car wishes to leave the car park. A sign at the entrance display “Full” when there are no spaces in the car park. It is only switched off when a car leaves. There is another type of card for guests, which also permits access to the car park. The card records a number and the current date. Such cards may be sent out in advance, or collected from reception. All guest cards must be returned to reception when the visitor leaves Saturn Int."
    },
    {
        "title": "OOBank",
        "desc": "This system provides the basic services to manage bank accounts at a bank called OOBank. OOBank has many branches, each of which has an address and branch number. A client opens accounts at a branch. Each account is uniquely identified by an account number; it has a balance and a credit or overdraft limit. There are many types of accounts, including: a mortgage account (which has a property as collateral), a checking account, and a credit card account (which has an expiry date and can have secondary cards attached to it). It is possible to have a joint account (e.g. for a husband and wife). Each type of account has a particular interest rate, a monthly fee and a specific set of privileges (e.g. ability to write checks, insurance for purchases etc.). OOBank is divided into divisions and subdivisions (such as Planning, Investments and Consumer); the branches are considered subdivisions of the Consumer Division. Each division has a manager and a set of other employees. Each customer is assigned a particular employee as his or her ‘personal banker’."
    },
    {
        "title": "Prepaid Cell Phone",
        "desc": "The contract of a prepaid cell phone should be modelled and implemented. A basic contract has a contract number (of type int) and a balance (of type double), but no monthly charges. The contract number is not automatically generated, but is to be set as a parameter by the constructor as well as the initial balance. The balance has a getter and a setter. The following options can be added to a contract (if needed also several times): ● 100 MB of data (monthly charge 1.00€) ● 50 SMS (monthly charge 0.50€) ● 50 minutes (monthly charge 1.50€) ● Double Transfer Rate (monthly charge 2.00€) implement this requirement with the help of the decorator pattern. All contract elements should be able to understand the methods getCharges():double, getBalance():double and setBalance(double). The method getCharges() should provide the monthly charge of a contract with all its options selected. The methods getBalance() and setBalance() should be passed through and access the basic contract."
    },
    {
        "title": "Library System",
        "desc": "The exercise id to design a class structure for a library system. It should fulfil those requirements: ● There are two type of users – under-aged and adults. ● Under-aged users are identified with usage of their full name and student card. ● Adult users are identified with usage of their full name and ID card. ● The library contains books. ● There is basic information about every book (title, author, etc). ● The user can borrow at most 4 books at the same time. ● There is a history of previously borrowed books for every user (along with all the dates)."
    },
    {
        "title": "MyDoctor",
        "desc": "The MyDoctor application aims to be a management tool for the appointments of a doctor. A hospital has multiple offices. The users of the application can be doctors and patients. The doctors can apply to practice in offices and create a schedule for an office. The schedules in different offices can’t overlay. 📝 Example: Doctor Ana is available in Office 4 on the 4th of September during 1 PM - 5PM. Doctor Ana can’t practice in Office 5 on the 4th of September during 3PM - 8 PM, but she can practice in Office 5 on the 4th of September during 5:30PM - 8 PM. The patients can see the existing doctors in the system, the schedule of the offices and can book appointments for specific doctors and for specific schedules. The appointments can be of 3 types: ● Blood Test - 15 mins ● Consultation - 30 mins ● Surgery - 60 mins The booking of an appointment will not be possible if another appointment is already booked at the same time frame. An email is sent to the patient with the confirmation of the appointment. 📝 Example: Action 1: User Mike will create a blood test booking for Doctor Ana for the 4th of September starting with 15:30 PM → Possible Action 2: User Mike will create an intervention booking for Doctor Ana for the 4th of September starting with 15:00 PM → Not Possible Action 3: User Mike will create an intervention booking for Doctor Ana for the 4th of September starting with 16:00 PM → Possible"
    },
    {
        "title": "Online Shopping",
        "desc": "Each customer has unique id and is linked to exactly one account. Account owns shopping cart and orders. Customer could register as a web user to be able to buy items online. Customer is not required to be a web user because purchases could also be made by phone or by ordering from catalogues. Web user has login name which also serves as unique id. Web user could be in several states - new, active, temporary blocked, or banned, and be linked to a shopping cart. Shopping cart belongs to account. Account owns customer orders. Customer may have no orders. Customer orders are sorted and unique. Each order could refer to several payments, possibly none. Every payment has unique id and is related to exactly one account. Each order has current order status. Both order and shopping cart have line items linked to a specific product. Each line item is related to exactly one product. A product could be associated to many line items or no item at all."
    }
]

# --------------------------------------------------------------------
# CONFIGURATION
# --------------------------------------------------------------------

# Ensure you have set this in your environment: export OPENAI_API_KEY="sk-..."
OPENAI_API_KEY = "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"
OPENAI_API_URL = "https://api.openai.com/v1/chat/completions"
PLANTUML_SERVER_URL = "http://www.plantuml.com/plantuml/png/"

# Output configuration
OUTPUT_DIR = "mcet"
os.makedirs(OUTPUT_DIR, exist_ok=True)
REPORT_FILE = os.path.join(OUTPUT_DIR, "mcet_report.txt")
DIAGRAM_CODE_FILE = os.path.join(OUTPUT_DIR, "final_diagram.puml")

# --------------------------------------------------------------------
# 1. LLM CALLER & UTILS
# --------------------------------------------------------------------

def call_llm(prompt: str, system: str = None, model: str = "gpt-4o-mini") -> str:
    """Single call to LLM. No voting."""
    if not OPENAI_API_KEY:
        raise ValueError("OPENAI_API_KEY not set.")

    headers = {
        "Content-Type": "application/json",
        "Authorization": f"Bearer {OPENAI_API_KEY}",
    }

    messages = []
    if system:
        messages.append({"role": "system", "content": system})
    messages.append({"role": "user", "content": prompt})

    payload = {
        "model": model,
        "messages": messages,
        "temperature": 0.2, 
        "max_tokens": 4000
    }

    try:
        resp = requests.post(OPENAI_API_URL, headers=headers, json=payload, timeout=60)
        resp.raise_for_status()
        content = resp.json()["choices"][0]["message"]["content"]
        return content
    except Exception as e:
        print(f"[LLM ERROR] {e}")
        return ""

def clean_json_response(text: str):
    """Extracts JSON from Markdown code blocks if present."""
    text = text.strip()
    if text.startswith("```json"):
        text = text.replace("```json", "").replace("```", "")
    elif text.startswith("```"):
        text = text.replace("```", "")
    return text.strip()

def log_step(step_name, content):
    """Prints progress and logs LLM response."""
    print(f"\n{'='*20} {step_name} {'='*20}")
    print(content)
    print("="*50)

# --------------------------------------------------------------------
# 2. ATOMIZATION (LLM BASED)
# --------------------------------------------------------------------

def get_requirement_atoms(requirements_text: str) -> list[str]:
    print("...Splitting Requirements into Atoms...")
    prompt = f"""
    Split the following SOFTWARE REQUIREMENTS text into a list of atomic requirement sentences.
    Each atom should represent a single verifiable fact or action.
    
    Return ONLY a raw JSON list of strings. No markdown formatting.
    
    Requirements:
    {requirements_text}
    """
    resp = call_llm(prompt)
    log_step("Requirement Atoms LLM Response", resp)
    
    try:
        return json.loads(clean_json_response(resp))
    except:
        return [requirements_text] # Fallback

def get_diagram_atoms(cot_text: str) -> list[str]:
    print("...Splitting CoT/Diagram into Atoms...")
    prompt = f"""
    The following text describes a Class Diagram (Classes, Attributes, Methods, Relationships).
    Split this text into "Diagram Atoms". 
    A Diagram Atom is a specific assertion about the design, e.g., "Class User has attribute email" or "User is related to Order".
    
    Return ONLY a raw JSON list of strings. No markdown formatting.
    
    CoT Description:
    {cot_text}
    """
    resp = call_llm(prompt)
    log_step("Diagram Atoms LLM Response", resp)
    
    try:
        return json.loads(clean_json_response(resp))
    except:
        return [cot_text]

# --------------------------------------------------------------------
# 3. MCeT CHECKS
# --------------------------------------------------------------------

def check_holistic(req_text: str, cot_text: str) -> list[dict]:
    print("...Running Holistic Check...")
    prompt = f"""
    You are a Senior Architect. Compare the REQUIREMENTS against the DESIGN DESCRIPTION (CoT).
    Identify high-level discrepancies, missing concepts, or logic errors.
    
    Return ONLY a raw JSON list of objects with fields: 
    {{"type": "accuracy"|"completeness", "issue": "description of issue", "severity": "high"|"low"}}
    
    Requirements:
    {req_text}
    
    Design Description:
    {cot_text}
    """
    resp = call_llm(prompt)
    log_step("Holistic Check Response", resp)
    try:
        return json.loads(clean_json_response(resp))
    except:
        return []

def check_diagram_atoms(req_text: str, diagram_atoms: list[str]) -> list[dict]:
    print("...Running Diagram Atom Checks...")
    # To save time/tokens, we verify atoms in a batch, but instruct LLM to check them individually
    prompt = f"""
    Given the REQUIREMENTS, verify the validity of the following DESIGN ATOMS.
    For each atom, determine if it is "Consistent", "Contradictory", or "Spurious" (not in reqs, but maybe needed for code).
    
    Return ONLY a raw JSON list of objects:
    {{"atom": "text of atom", "status": "Consistent"|"Contradictory"|"Spurious", "note": "explanation"}}
    
    Requirements:
    {req_text}
    
    Design Atoms:
    {json.dumps(diagram_atoms)}
    """
    resp = call_llm(prompt)
    log_step("Diagram Atom Check Response", resp)
    try:
        return json.loads(clean_json_response(resp))
    except:
        return []

def check_req_atoms(req_atoms: list[str], cot_text: str) -> list[dict]:
    print("...Running Requirement Atom Checks (The Authority)...")
    prompt = f"""
    Given the DESIGN DESCRIPTION, check if each REQUIREMENT ATOM is implemented.
    This check is the 'Higher Authority'.
    
    Return ONLY a raw JSON list of objects:
    {{"req_atom": "text", "implemented": true|false, "missing_details": "explanation if false"}}
    
    Design Description:
    {cot_text}
    
    Requirement Atoms:
    {json.dumps(req_atoms)}
    """
    resp = call_llm(prompt)
    log_step("Req Atom Check Response", resp)
    try:
        return json.loads(clean_json_response(resp))
    except:
        return []

# --------------------------------------------------------------------
# 4. CROSS CHECK & FILTERING
# --------------------------------------------------------------------

def cross_check_results(holistic, diag_atom_results, req_atom_results):
    print("...Cross Checking and Filtering Issues...")
    
    # 1. Establish Authority: What is definitely implemented?
    implemented_reqs = [r['req_atom'] for r in req_atom_results if r['implemented'] is True]
    missing_reqs = [r for r in req_atom_results if r['implemented'] is False]
    
    # 2. Gather candidate issues from other checks
    candidate_issues = []
    
    # From Holistic
    for h in holistic:
        candidate_issues.append(f"Holistic Issue: {h['issue']}")
        
    # From Diagram Atoms (only include contradictions, spurious might be technical detail)
    for d in diag_atom_results:
        if d['status'] == 'Contradictory':
            candidate_issues.append(f"Diagram Contradiction: {d['atom']} - {d['note']}")

    # 3. LLM Filter: Filter candidate issues based on Implemented Reqs
    if not candidate_issues:
        filtered_issues = []
    else:
        prompt = f"""
        You are the MCeT Filter. 
        I have a list of CONFIRMED IMPLEMENTED REQUIREMENTS (The Authority).
        I have a list of CANDIDATE ISSUES reported by other checks.
        
        Task: Filter out any Candidate Issue that contradicts a Confirmed Requirement. 
        (e.g., if Candidate says "X is missing" but Confirmed says "Requirement X is implemented", remove the candidate).
        
        Return ONLY a raw JSON list of the valid remaining issues strings.
        
        Confirmed Implemented Reqs:
        {json.dumps(implemented_reqs)}
        
        Candidate Issues:
        {json.dumps(candidate_issues)}
        """
        resp = call_llm(prompt)
        log_step("Cross-Check Filter Response", resp)
        try:
            filtered_issues = json.loads(clean_json_response(resp))
        except:
            filtered_issues = candidate_issues

    # 4. Compile Final Report Data
    final_report_data = {
        "verified_implemented_requirements": implemented_reqs,
        "missing_requirements": [r['req_atom'] for r in missing_reqs],
        "design_issues": filtered_issues
    }
    return final_report_data

# --------------------------------------------------------------------
# 5. PLANTUML GENERATION & RENDERING
# --------------------------------------------------------------------

def generate_plantuml_code(cot_text: str) -> str:
    print("...Generating PlantUML Code...")
    prompt = f"""
    You will be given a textual description of a system with classes.
    Convert this description into valid PlantUML class diagram syntax.
    
    Rules: 
    - Output must start with @startuml and end with @enduml.
    - No markdown formatting like ```plantuml. Just the code.
    - Include classes, attributes, methods, and relationships.
    
    Description:
    {cot_text}
    """
    code = call_llm(prompt)
    
    # Clean up standard markdown if LLM ignores instruction
    code = clean_json_response(code) # re-using cleaner to strip ticks
    match_start = code.find("@startuml")
    match_end = code.find("@enduml") + len("@enduml")
    
    if match_start != -1 and match_end != -1:
        clean_code = code[match_start:match_end]
    else:
        clean_code = code
        
    log_step("Generated PlantUML Code", clean_code)
    return clean_code

def render_diagram(plantuml_code: str, filename: str):
    print(f"...Rendering Diagram to {filename}...")
    server = PlantUML(url=PLANTUML_SERVER_URL)
    
    try:
        # processes() returns the binary png data
        png_data = server.processes(plantuml_code)
        with open(filename, "wb") as f:
            f.write(png_data)
        print(f"[OK] Diagram saved to {filename}")
    except Exception as e:
        print(f"[ERROR] Failed to render diagram: {e}")

# --------------------------------------------------------------------
# MAIN PIPELINE
# --------------------------------------------------------------------

# def run_pipeline(req_text, cot_text):
#     print(f"Starting MCeT Pipeline. Output: {OUTPUT_DIR}/")
    
#     # 1. Atomize
#     req_atoms = get_requirement_atoms(req_text)
#     diag_atoms = get_diagram_atoms(cot_text)
    
#     # 2. Run Checks
#     holistic_res = check_holistic(req_text, cot_text)
#     diag_atom_res = check_diagram_atoms(req_text, diag_atoms)
#     req_atom_res = check_req_atoms(req_atoms, cot_text)
    
#     # 3. Cross Check
#     final_analysis = cross_check_results(holistic_res, diag_atom_res, req_atom_res)
    
#     # 4. Generate Visuals
#     uml_code = generate_plantuml_code(cot_text)
    
#     # Save UML Text
#     with open(DIAGRAM_CODE_FILE, "w") as f:
#         f.write(uml_code)
        
#     # Render PNG
#     png_path = os.path.join(OUTPUT_DIR, "class_diagram.png")
#     render_diagram(uml_code, png_path)
    
#     # 5. Write Final Report
#     with open(REPORT_FILE, "w") as f:
#         f.write("=== MCeT ANALYSIS REPORT ===\n\n")
        
#         f.write("--- 1. CONFIRMED IMPLEMENTED REQUIREMENTS ---\n")
#         for r in final_analysis['verified_implemented_requirements']:
#             f.write(f"[OK] {r}\n")
            
#         f.write("\n--- 2. MISSING REQUIREMENTS ---\n")
#         if not final_analysis['missing_requirements']:
#             f.write("None detected.\n")
#         for r in final_analysis['missing_requirements']:
#             f.write(f"[MISSING] {r}\n")
            
#         f.write("\n--- 3. DETECTED DESIGN ISSUES (Filtered) ---\n")
#         if not final_analysis['design_issues']:
#             f.write("No major contradictions found.\n")
#         for issue in final_analysis['design_issues']:
#             f.write(f"[ISSUE] {issue}\n")
            
#     print(f"\nPipeline Finished. Check the '{OUTPUT_DIR}' folder.")


def run_pipeline(req_text, cot_text, output_dir):
    # Create per-example output folder
    os.makedirs(output_dir, exist_ok=True)

    report_file = os.path.join(output_dir, "mcet_report.txt")
    diagram_code_file = os.path.join(output_dir, "final_diagram.puml")
    diagram_png_file = os.path.join(output_dir, "class_diagram.png")

    print(f"Starting MCeT Pipeline. Output: {output_dir}/")
    
    # 1. Atomize
    req_atoms = get_requirement_atoms(req_text)
    diag_atoms = get_diagram_atoms(cot_text)
    
    # 2. Run Checks
    holistic_res = check_holistic(req_text, cot_text)
    diag_atom_res = check_diagram_atoms(req_text, diag_atoms)
    req_atom_res = check_req_atoms(req_atoms, cot_text)
    
    # 3. Cross Check
    final_analysis = cross_check_results(holistic_res, diag_atom_res, req_atom_res)
    
    # 4. Generate Visuals (PlantUML)
    uml_code = generate_plantuml_code(cot_text)
    
    # Save UML (PlantUML) Text
    with open(diagram_code_file, "w", encoding="utf-8") as f:
        f.write(uml_code)
        
    # Render PNG
    render_diagram(uml_code, diagram_png_file)
    
    # 5. Write Final Report
    with open(report_file, "w", encoding="utf-8") as f:
        f.write("=== MCeT ANALYSIS REPORT ===\n\n")
        
        f.write("--- 1. CONFIRMED IMPLEMENTED REQUIREMENTS ---\n")
        for r in final_analysis['verified_implemented_requirements']:
            f.write(f"[OK] {r}\n")
            
        f.write("\n--- 2. MISSING REQUIREMENTS ---\n")
        if not final_analysis['missing_requirements']:
            f.write("None detected.\n")
        for r in final_analysis['missing_requirements']:
            f.write(f"[MISSING] {r}\n")
            
        f.write("\n--- 3. DETECTED DESIGN ISSUES (Filtered) ---\n")
        if not final_analysis['design_issues']:
            f.write("No major contradictions found.\n")
        for issue in final_analysis['design_issues']:
            f.write(f"[ISSUE] {issue}\n")
            
    print(f"\nPipeline Finished. Check the '{output_dir}' folder.")

# --------------------------------------------------------------------
# EXECUTION
# --------------------------------------------------------------------



# if __name__ == "__main__":

    
    # INPUTS
    
    # requirements_input = (
    #     "Design a system for a movie-shop. Users can browse the catalogue. "
    #     "Subscribers have a rechargeable card. Only subscribers can rent movies. "
    #     "Credit is deducted from the card when renting. "
    #     "If a movie is not available, it is ordered (back-ordered). "
    #     "Both users and subscribers can buy movies."
    # )
    
    # cot_input = """
    # CLASS: Movie
    # Attributes: title, movieId, availableQuantity
    # Methods: getTitle(), rent()
    
    # CLASS: User
    # Attributes: userId, name, email
    
    # CLASS: Subscriber (inherits User)
    # Attributes: subscriptionDate, card: RechargeableCard
    # Methods: rentMovie(movie), checkCredit()
    
    # CLASS: RechargeableCard
    # Attributes: balance
    # Methods: deduct(amount)
    
    # CLASS: Order
    # Attributes: orderDate, totalAmount
    
    # RELATIONSHIPS:
    # - Subscriber has 1 RechargeableCard (composition)
    # - Subscriber rents Movie
    # - User places Order
    # """

    # # RUN
    # run_pipeline(requirements_input, cot_input)



if __name__ == "__main__":
    # Load textual UMLs (from your json_results.json example)
    with open(JSON_RESULTS_FILE, "r", encoding="utf-8") as jf:
        cot_results = json.load(jf)

    # Sanity: assume same ordering as all_descriptions
    for idx, desc_obj in enumerate(all_descriptions):
        title = desc_obj["title"]
        requirements_input = desc_obj["desc"]

        # get matching textual UML from JSON
        cot_entry = cot_results[idx]
        cot_text_raw = cot_entry["textual_uml"]
        # remove ``` fences if present
        cot_text = clean_json_response(cot_text_raw)

        slug = slugify(title)
        example_dir = os.path.join(OUTPUT_DIR, slug)

        # Save textual UML as a plain .txt for this example
        os.makedirs(example_dir, exist_ok=True)
        textual_uml_path = os.path.join(example_dir, "textual_uml.txt")
        with open(textual_uml_path, "w", encoding="utf-8") as tf:
            tf.write(cot_text)

        print("\n" + "#" * 70)
        print(f"Running MCeT pipeline for: {title}")
        print("#" * 70)

        # Run pipeline for this example (req + textual UML)
        run_pipeline(requirements_input, cot_text, example_dir)
